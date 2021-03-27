/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author mrk
 */

#include <iostream>
#include <epicsGetopt.h>
#include <pv/pvaClientMultiChannel.h>
#include <pv/convert.h>

using std::tr1::static_pointer_cast;
using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaClient;

class MultiMonitorDouble;
typedef std::tr1::shared_ptr<MultiMonitorDouble> MultiMonitorDoublePtr;

class MultiMonitorDouble :
    public std::tr1::enable_shared_from_this<MultiMonitorDouble>
{
private:
    PvaClientMultiChannelPtr multiChannel;
    PvaClientMultiMonitorDoublePtr multiMonitorDouble;
    shared_vector<const std::string> const channelNames;
    shared_vector<const std::string> const providerNames;

    MultiMonitorDouble(
       shared_vector<const string> const &channelNames,
       shared_vector<const std::string> const providerNames
     )
    : 
      channelNames(channelNames),
      providerNames(providerNames)
    {}

    void init(PvaClientPtr const &pva)
    {
        multiChannel = PvaClientMultiChannel::create(pva,channelNames,"pva",0,providerNames);
        Status status = multiChannel->connect();
        if(!status.isSuccess()) {
             cout << "Did not connect: ";
             shared_vector<epics::pvData::boolean> isConnected = multiChannel->getIsConnected();
             size_t num = channelNames.size();
             for(size_t i=0; i<num; ++i) {
                 if(!isConnected[i]) cout << channelNames[i] << " ";
             }
             cout << endl;
        }
        multiMonitorDouble = multiChannel->createMonitor();
        multiMonitorDouble->connect();
    }
public:     
    static MultiMonitorDoublePtr create(
        PvaClientPtr const &pva,
        shared_vector<const string> const &channelNames,
        shared_vector<const string> const &providerNames
    )
    {
        
        MultiMonitorDoublePtr clientMonitorDouble(MultiMonitorDoublePtr(
            new MultiMonitorDouble(channelNames,providerNames)));
        clientMonitorDouble->init(pva);
        return clientMonitorDouble;
    }
    void poll()
    {
        bool result = multiMonitorDouble->waitEvent(.1);
        while(result) {
            epics::pvData::shared_vector<double> data(multiMonitorDouble->get());
            cout << "value=" << data << "\n";
            result = multiMonitorDouble->poll();
        }
    }
};


int main(int argc,char *argv[])
{
    string provider("pva");
    shared_vector<string> providerNames;
    shared_vector<string> channelNames;
    channelNames.push_back("DBRdouble");
    providerNames.push_back("ca");
    channelNames.push_back("DBRint00");
    providerNames.push_back("ca");
    channelNames.push_back("PVRbyte");
    channelNames.push_back("PVRshort");
    channelNames.push_back("PVRint");
    channelNames.push_back("PVRlong");
    channelNames.push_back("PVRfloat");
    channelNames.push_back("PVRdouble");
    string debugString;
    bool debug(false);
    int opt;
    while((opt = getopt(argc, argv, "hp:d:")) != -1) {
        switch(opt) {
            case 'h':
             cout << " -h -p provider -d debug  \"channel provider\" ... channel ... " << endl;
             cout << "default" << endl;
             cout << " -d " 
                  << " " <<  channelNames
                  << endl;           
                return 0;
            case 'p':
                provider = optarg;
                break;  
            case 'd' :
               debugString =  optarg;
               if(debugString=="true") debug = true;
               break;
            default:
                std::cerr<<"Unknown argument: "<<opt<<"\n";
                return -1;
        }
    }
    cout << "_____multiMonitorDouble starting_______\n";
    if(debug) PvaClient::setDebug(true);
    try {
        int nPvs = argc - optind;       /* Remaining arg list are PV names */
        if (nPvs!=0)
        {
            channelNames.clear();
            providerNames.clear();
            while(optind < argc) {
                string value(argv[optind]);
                size_t offset = value.find(" ",0);
                if(offset==string::npos) {
                    channelNames.push_back(value);
                } else {
                    channelNames.push_back(value.substr(0,offset));
                    providerNames.push_back(value.substr(offset+1));
                }
                optind++;
            }
        }
        cout << " channelNames " <<  channelNames << endl;
        PvaClientPtr pva= PvaClient::get("pva ca");
        shared_vector<const string> names(freeze(channelNames));
        shared_vector<const string> providers(freeze(providerNames));
        MultiMonitorDoublePtr clientMonitorDouble(MultiMonitorDouble::create(pva,names,providers));
        while(true) {
            cout << "Type exit or enter to poll\n";
            string str;
            getline(cin,str);
            if(str.compare("exit")==0) break;
            clientMonitorDouble->poll();
        }
        cout << "_____multiMonitorDouble done_______\n";
    } catch (std::exception& e) {
        cout << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
