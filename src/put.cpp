/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author mrk
 */

/* Author: Marty Kraimer */
#include <iostream>
#include <epicsGetopt.h>
#include <epicsGuard.h>
#include <pv/pvaClient.h>
#include <pv/convert.h>
#include <epicsThread.h>
#include <pv/timeStamp.h>

using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaClient;

typedef epicsGuard<epicsMutex> Guard;

class ClientPut;
typedef std::tr1::shared_ptr<ClientPut> ClientPutPtr;

class ClientPut :
    public PvaClientChannelStateChangeRequester,
    public PvaClientPutRequester,
    public std::tr1::enable_shared_from_this<ClientPut>
{
private:
    string channelName;
    string providerName;
    string request;
    bool channelConnected;
    bool putConnected;
    size_t num;
    long numPut;
    epicsMutex mutex;

    PvaClientChannelPtr pvaClientChannel;
    PvaClientPutPtr pvaClientPut;

    void init(PvaClientPtr const &pvaClient)
    {
        pvaClientChannel = pvaClient->createChannel(channelName,providerName);
        pvaClientChannel->setStateChangeRequester(shared_from_this());
        pvaClientChannel->issueConnect();
    }
public:
    POINTER_DEFINITIONS(ClientPut);
    ClientPut(
        const string &channelName,
        const string &providerName,
        const string &request,
        size_t num)
    : channelName(channelName),
      providerName(providerName),
      request(request),
      channelConnected(false),
      putConnected(false),
      num(num),
      numPut(0)
    {
    }
    
    static ClientPutPtr create(
        PvaClientPtr const &pvaClient,
        const string & channelName,
        const string & providerName,
        const string  & request,
        size_t num)
    {
        ClientPutPtr client(ClientPutPtr(
             new ClientPut(channelName,providerName,request,num)));
        client->init(pvaClient);
        return client;
    }

    virtual void channelStateChange(PvaClientChannelPtr const & channel, bool isConnected)
    {
        channelConnected = isConnected;
        if(isConnected) {
            if(!pvaClientPut) {
                pvaClientPut = pvaClientChannel->createPut(request);
                pvaClientPut->setRequester(shared_from_this());
                pvaClientPut->issueConnect();
            }
        }
    }

    virtual void channelPutConnect(
        const epics::pvData::Status& status,
        PvaClientPutPtr const & clientPut)
    {
        putConnected = true;
        cout << "channelPutConnect " << channelName << " status " << status << endl;
    }

    
    virtual void putDone(
        const epics::pvData::Status& status,
        PvaClientPutPtr const & clientPut)
    {
         if(!status.isOK()) {
             cout << "putDone " << channelName << " status " << status << endl;
         } else {
             numPut++;
             {
                Guard G(mutex);
                numPut++;
            }
         }
    }

    void put(double value)
    {
        if(!channelConnected) {
            cout << channelName << " channel not connected\n";
            return;
        }
        if(!putConnected) {
            cout << channelName << " channelPut not connected\n";
            return;
        }
        PvaClientPutDataPtr putData = pvaClientPut->getData();
        shared_vector<double> data(num,0);
        for(size_t i=0; i<data.size(); ++i) data[i] = (value +1);
        putData->putDoubleArray(freeze(data));
        pvaClientPut->put();
    }

    long report()
    {
        long numnow = 0;
        {
            Guard G(mutex);
            numnow = numPut;
            numPut = 0;
        }
        return numnow;
    }
};


int main(int argc,char *argv[])
{
    string provider("pva");
    string channelName("DBRdoubleArray01");
    string request("value");
    string debugString;
    bool debug(false);
    int opt;
    while((opt = getopt(argc, argv, "hp:r:d:")) != -1) {
        switch(opt) {
            case 'p':
                provider = optarg;
                break;
            case 'r':
                request = optarg;
                break;
            case 'h':
             cout << " -h -p provider -r request - d debug channelNames " << endl;
             cout << "default" << endl;
             cout << "-p " << provider 
                  << " -r " << request
                  << " -d " << (debug ? "true" : "false") 
                  << " " <<  channelName
                  << endl;           
                return 0;
            case 'd' :
               debugString =  optarg;
               if(debugString=="true") debug = true;
               break;
            default:
                std::cerr<<"Unknown argument: "<<opt<<"\n";
                return -1;
        }
    }
    bool pvaSrv(((provider.find("pva")==string::npos) ? false : true));
    bool caSrv(((provider.find("ca")==string::npos) ? false : true));
    if(pvaSrv&&caSrv) {
        cerr<< "multiple providers are not allowed\n";
        return 1;
    }
    cout << "provider " << provider
         << " channelName " <<  channelName
         << " request " << request
         << " debug " << (debug ? "true" : "false")  << endl;

    cout << "_____put starting__\n";
    
    try {   
        if(debug) PvaClient::setDebug(true);
        size_t nelements = 1000000;
        vector<string> channelNames;
        vector<ClientPutPtr> ClientPuts;
        int nPvs = argc - optind;       /* Remaining arg list are PV names */
        if (nPvs==0)
        {
            channelNames.push_back(channelName);
            nPvs = 1;
        } else {
            for (int n = 0; optind < argc; n++, optind++) channelNames.push_back(argv[optind]);
        }
        PvaClientPtr pva= PvaClient::get(provider);
        for(int i=0; i<nPvs; ++i) {
            ClientPuts.push_back(ClientPut::create(pva,channelNames[i],provider,request,nelements));
        }
        double value = 0.0;
        epicsThreadSleep(1.0);
        TimeStamp timeStamp;
        TimeStamp timeStampLast;
        timeStampLast.getCurrent();
        while(true) {
            for(int i=0; i<nPvs; ++i) {
                try {
                    ClientPuts[i]->put(value++);
                } catch(std::exception& e) {
                   cerr << "exception " << e.what() << endl;
                }
            }
            timeStamp.getCurrent();
            double diff = TimeStamp::diff(timeStamp,timeStampLast);
            if(diff>2.0) {
                 long total = 0;
                 for(int i=0; i<nPvs; ++i) {
                     total += ClientPuts[i]->report();
                 }
                 cout << "puts per second " << (total/diff) << endl;
                 timeStampLast.getCurrent();
            }
        }
    } catch(std::exception& e) {
        cerr << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
