/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author mrk
 */

/* Author: Marty Kraimer */
#include <iostream>

#include <pv/pvaClient.h>
#include <pv/convert.h>

using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaClient;

class ClientPut;
typedef std::tr1::shared_ptr<ClientPut> ClientPutPtr;

class ClientPut :
    public PvaClientChannelStateChangeRequester,
    public PvaClientPutRequester,
    public std::tr1::enable_shared_from_this<ClientPut>
{
private:
    string request;
    PvaClientChannelPtr pvaClientChannel;
    PvaClientPutPtr pvaClientPut;
    PvaClientPutDataPtr putData;
public:
    POINTER_DEFINITIONS(ClientPut);
    ClientPut(
        const string &request,
        const PvaClientChannelPtr & pvaClientChannel)
    : request(request),
      pvaClientChannel(pvaClientChannel)
    {
    }
    
    static ClientPutPtr create(
        PvaClientPtr const &pvaClient,
        const string & channelName,
        const string & providerName,
        const string  & request)
    {
        PvaClientChannelPtr pvaClientChannel = pvaClient->createChannel(channelName,providerName);
        ClientPutPtr clientPut(new ClientPut(request,pvaClientChannel));
        pvaClientChannel->setStateChangeRequester(clientPut);
        pvaClientChannel->issueConnect();
        return clientPut;
    }

    virtual void channelStateChange(PvaClientChannelPtr const & channel, bool isConnected)
    {
        if(isConnected&&!pvaClientPut)
        {
           pvaClientPut  = pvaClientChannel->createPut(request);
           pvaClientPut->setRequester(shared_from_this());
           pvaClientPut->issueConnect();
        }
    }

    virtual void channelPutConnect(
        const epics::pvData::Status& status,
        PvaClientPutPtr const & clientPut)
    {
        cout << "channelPutConnect status " << status << endl;
    }

    
    virtual void putDone(
        const epics::pvData::Status& status,
        PvaClientPutPtr const & clientPut)
    {
         cout << "putDone status " << status << endl;
    }

    PvaClientPutPtr getPvaClientPut()
    {
        return  pvaClientPut;
    }

    PvaClientPutDataPtr getData()
    {
         if(!putData) putData = pvaClientPut->getData();
         return putData;
    }
};


int main(int argc,char *argv[])
{
    string providerName("pva");
    string channelName("DBRdoubleArray");
    string request("value");
    bool debug(false);
    if(argc==2 && string(argv[1])==string("-help")) {
        cout << "providerName channelName request debug" << endl;
        cout << "default" << endl;
        cout << providerName << " " <<  channelName
             << " " << '"' << request << '"'
             << " " << (debug ? "true" : "false") << endl;
        return 0;
    }
    if(argc>1) providerName = argv[1];
    if(argc>2) channelName = argv[2];
    if(argc>3) request = argv[3];
    if(argc>4) {
        string value(argv[4]);
        if(value=="true") debug = true;
    }
    bool pvaSrv(((providerName.find("pva")==string::npos) ? false : true));
    bool caSrv(((providerName.find("ca")==string::npos) ? false : true));
    if(pvaSrv&&caSrv) {
        cerr<< "multiple providerNames are not allowed\n";
        return 1;
    }
    cout << "providerName " << providerName
         << " pvaSrv " << (pvaSrv ? "true" : "false")
         << " caSrv " << (caSrv ? "true" : "false")
         << " channelName " <<  channelName
         << " request " << request
         << " debug " << (debug ? "true" : "false") << endl;

    cout << "_____putNoBlock starting__\n";
    
    try {   
        if(debug) PvaClient::setDebug(true);
        ConvertPtr convert = getConvert();
        PvaClientPtr pva= PvaClient::get(providerName);
        ClientPutPtr clientPut(ClientPut::create(pva,channelName,providerName,request));
        double nextvalue = 0.0;
        while(true) {
            int c = std::cin.peek();  // peek character
            if ( c == EOF ) continue;
            cout << "Type exit to stop: \n";
            string str;
            getline(cin,str);
            if(str.compare("exit")==0) break;
            PvaClientPutPtr pvaClientPut = clientPut->getPvaClientPut();
            if(!pvaClientPut) {
                cout << "not connected\n";
            } else {
                try {
                    size_t n = 5;
                    shared_vector<double> value(n);
                    for(size_t i=0; i<n; ++i) {
                        value[i] = nextvalue;
                        nextvalue += 1.0;
                    }
                    pvaClientPut->getData()->putDoubleArray(freeze(value));
                    pvaClientPut->put();
                } catch (std::runtime_error e) {
                     cout << "str " << str << " is not a valid value\n";
                }
            }
        }
    } catch (std::runtime_error e) {
        cerr << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
