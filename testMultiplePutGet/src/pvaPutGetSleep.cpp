/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author Marty Kraimer
 * @date 2021.02
 */

#include <iostream>
#include <time.h>
#include <unistd.h>
#include <epicsGetopt.h>
#include <epicsThread.h>
#include <pv/pvAccess.h>
#include <pv/clientFactory.h>
#include <pv/caProvider.h>
#include <pv/convert.h>
#include <pv/createRequest.h>

using std::tr1::static_pointer_cast;
using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvAccess::ca;

class ExampleRequester;
typedef std::tr1::shared_ptr<ExampleRequester> ExampleRequesterPtr;

class ExampleRequester :
    public ChannelRequester,
    public ChannelPutRequester,
    public ChannelGetRequester,
    public std::tr1::enable_shared_from_this<ExampleRequester>
{
private:
    const string channelName;
public:
    bool channelConnected = false;
    bool channelPutConnected = false;
    bool channelPutDone = false;
    bool channelGetConnected = false;
    bool channelGetDone = false;
    Channel::shared_pointer theChannel;
    ChannelPut::shared_pointer theChannelPut;
    Structure::const_shared_pointer theChannelPutStructure;
    ChannelGet::shared_pointer theChannelGet;
    PVStructure::shared_pointer theChannelGetPVStructure;
    ExampleRequester(const string & channelName)
    : channelName(channelName)
    {} 
    
    virtual std::string getRequesterName(){
        throw std::runtime_error("getRequesterName not implemented");
    }
    
    virtual void channelCreated(const Status& status, Channel::shared_pointer const & channel)
    {
        if(status.isOK()) {return;}
        string message = string("channel ") + channelName + " channelCreated status=" + status.getMessage();
        throw std::runtime_error(message);       
    }
    
    virtual void channelStateChange(Channel::shared_pointer const & channel,
        Channel::ConnectionState connectionState)
    {  
         if(connectionState==Channel::CONNECTED) {
             channelConnected = true;
             theChannel = channel;
         } else {
             string message = string("channel ") + channelName + " connection state "
                + Channel::ConnectionStateNames[connectionState];
             throw std::runtime_error(message);
        }
    } 
        
    virtual void channelPutConnect(
        const Status& status,
        ChannelPut::shared_pointer const & channelPut,
        Structure::const_shared_pointer const & structure)
   {
        if(status.isOK()) {
            channelPutConnected = true;
            theChannelPut = channelPut;
            theChannelPutStructure = structure;         
            return;
        }
        string message = string("channel ") + channelName + " channelPutConnect status=" + status.getMessage();
        throw std::runtime_error(message);       
    }
    virtual void putDone(
        const Status& status,
        ChannelPut::shared_pointer const & channelPut)
    {
        channelPutDone = true;
        if(status.isOK()) {return;}
        cout << "channel=" << channelName << " channelPutDone status=" << status.getMessage() << "\n";
    }
    virtual void getDone(
        const Status& status,
        ChannelPut::shared_pointer const & channelPut,
        PVStructure::shared_pointer const & pvStructure,
        BitSet::shared_pointer const & bitSet)
    {
        string message = string("channel ") + channelName + " channelPut:get not implemented";
        throw std::runtime_error(message);   
    }
    virtual void channelGetConnect(
        const Status& status,
        ChannelGet::shared_pointer const & channelGet,
        Structure::const_shared_pointer const & structure)
    {
        if(status.isOK()) {
            channelGetConnected = true;
            theChannelGet = channelGet;
            return;
        }
        string message = string("channel ") + channelName + " channelGetConnect status=" + status.getMessage();
        throw std::runtime_error(message);           
    }
    virtual void getDone(
        const Status& status,
        ChannelGet::shared_pointer const & channelGet,
        PVStructure::shared_pointer const & pvStructure,
        BitSet::shared_pointer const & bitSet)
    {
        channelGetDone = true;
        theChannelGetPVStructure = pvStructure;   
        if(status.isOK()) {return;}
        cout << "channel=" << channelName << " channelGetDone status=" << status.getMessage() << "\n";
    }
};   

static PVDataCreatePtr pvDataCreate = getPVDataCreate();
static ConvertPtr convert = getConvert();

static void example(
     string providerName,
     shared_vector<const string> const &channelNames)
{
    ChannelProviderRegistry::shared_pointer channelRegistry(ChannelProviderRegistry::clients());
    if(providerName=="pva") {
         ClientFactory::start();
    } else if(providerName=="ca") {
         CAClientFactory::start();
    } else {
         cerr << "provider " << providerName  << " not known" << endl;
         throw std::runtime_error("unknown provider");
    }
    int num = channelNames.size();
    shared_vector<ExampleRequesterPtr> exampleRequester(num);
    for(int i=0; i<num; i++) {
        exampleRequester[i] = ExampleRequesterPtr(new ExampleRequester(channelNames[i]));
    }
    ChannelProvider::shared_pointer channelProvider;
    shared_vector<Channel::shared_pointer> channels(num);
    shared_vector<ChannelPut::shared_pointer> channelPuts(num);
    shared_vector<ChannelGet::shared_pointer> channelGets(num);
    channelProvider = channelRegistry->getProvider(providerName);
    for(int i=0; i<num; i++) {
        channels[i] = channelProvider->createChannel(
            channelNames[i],exampleRequester[i],ChannelProvider::PRIORITY_DEFAULT);
    }
    clock_t startTime;
    clock_t endTime;
    startTime = clock();
    while(true) {
        epicsThreadSleep(.1);
        endTime = clock();
        double seconds = (double)(endTime - startTime)/CLOCKS_PER_SEC;
        if(seconds>5.0) {
            for(int i=0; i<num; i++) {
                if(!exampleRequester[i]->channelConnected) {
                    cout << "channel=" << channelNames[i] << " connect failed " <<"\n";
                }
            }    
            throw std::runtime_error("connect exception");
        }
        int numConnected = 0;
        for(int i=0; i<num; i++) {
            if(exampleRequester[i]->channelConnected) numConnected++;
        }
        if(numConnected==num) break;
    }
    CreateRequest::shared_pointer createRequest(CreateRequest::create());
    PVStructurePtr pvRequest = createRequest->createRequest("value");
    for(int i=0; i<num; i++) {
        channelPuts[i] = channels[i]->createChannelPut(exampleRequester[i],pvRequest);
        channelGets[i] = channels[i]->createChannelGet(exampleRequester[i],pvRequest);
    }
    startTime = clock();
    while(true) {
        epicsThreadSleep(.1);
        endTime = clock();
        double seconds = (double)(endTime - startTime)/CLOCKS_PER_SEC;
        if(seconds>5.0) {
            for(int i=0; i<num; i++) {
                if(!exampleRequester[i]->channelGetConnected) {
                    cout << "channel=" << channelNames[i] << " channelGetConnected failed " <<"\n";
                }
            }    
            throw std::runtime_error("connect exception");
        }
        int numConnected = 0;
        for(int i=0; i<num; i++) {
            if(exampleRequester[i]->channelGetConnected) numConnected++;
        }
        if(numConnected==num) break;
    } 
    startTime = clock();
    while(true) {
        epicsThreadSleep(.1);
        endTime = clock();
        double seconds = (double)(endTime - startTime)/CLOCKS_PER_SEC;
        if(seconds>5.0) {
            for(int i=0; i<num; i++) {
                if(!exampleRequester[i]->channelPutConnected) {
                    cout << "channel=" << channelNames[i] << " channelPutConnected failed " <<"\n";
                }
            }    
            throw std::runtime_error("connect exception");
        }
        int numConnected = 0;
        for(int i=0; i<num; i++) {
            if(exampleRequester[i]->channelPutConnected) numConnected++;
        }
        if(numConnected==num) break;
    }
    int successCount = 0;
    int failedCount = 0;
    startTime = clock();
    int numiter = 10000;
    int value = 0;
    for(int i = 0; i< numiter; i+= 1) {
        bool correctData = true;
        value++;
        if(value>127) value = 0;
        string strValue = to_string(value);
        for(int j=0; j<num; j++) {
            PVStructurePtr pvStructure = pvDataCreate->createPVStructure(
                exampleRequester[j]->theChannelPutStructure);
            BitSetPtr bitSet(BitSetPtr(new BitSet(pvStructure->getNumberFields())));
            PVScalarPtr pvScalar(pvStructure->getSubField<PVScalar>("value"));
            bitSet->set(pvScalar->getFieldOffset());
            convert->fromString(pvScalar,strValue);  
            try {
                exampleRequester[j]->theChannelPut->put(pvStructure,bitSet);
            } catch (std::exception& e) {
                 cout << "i=" << i << " channelName=" << channelNames[j]
                     << " put exception " << e.what() << endl;
                 correctData = false;    
             }
        }
        int numTry = 0;
        while(true) {
            if(numTry>100) {
                throw std::runtime_error("put timed out");
            }
            int numDone = 0;
            for(int j=0; j<num; j++) {
                if(exampleRequester[j]->channelPutDone) numDone++;
            }
            if(numDone==num) break;
            epicsThreadSleep(.001);
        }     
        for(int j=0; j<num; j++) {
            try {
                exampleRequester[j]->theChannelGet->get();
            } catch (std::exception& e) {
                 cout << "i=" << i << " channelName=" << channelNames[j]
                     << " get exception " << e.what() << endl;    
            } 
        }
        numTry = 0;
        while(true) {
            if(numTry>100) {
                throw std::runtime_error("get timed out");
            }
            int numDone = 0;
            for(int j=0; j<num; j++) {
                if(exampleRequester[j]->channelGetDone) numDone++;
            }
            if(numDone==num) break;
            epicsThreadSleep(.001);
        }
        for(int j=0; j<num; j++) {    
            PVStructurePtr pvStructure = exampleRequester[j]->theChannelGetPVStructure;
            PVScalarPtr pvScalar = pvStructure->getSubField<PVScalar>("value");
            string getValue = convert->toString(pvScalar);
            if(strValue!=getValue){
                 cout << "i=" << i << " channelName=" << channelNames[j]
                     << " expected=" << strValue << " got=" << getValue << "\n";
                 correctData = false;
            }
        }
        for(int j=0; j<num; j++) {
            exampleRequester[j]->channelPutDone = false;
            exampleRequester[j]->channelGetDone = false;
        }
        if(correctData) {
            successCount++;
        } else {
            failedCount++;
        }
    }
    double seconds = (double)(clock() - startTime)/CLOCKS_PER_SEC;
    cout << "time=" << seconds << " per interation=" << seconds/numiter << "\n";
    cout << "SUCCESS COUNT: " << successCount << endl;
    cout << "FAILED  COUNT: " << failedCount << endl;
    channelGets.clear();
    channelPuts.clear();
    channels.clear();
    exampleRequester.clear();
    if(providerName=="pva") {
         ClientFactory::stop();
    } else {
         CAClientFactory::stop();
    }
    channelRegistry.reset();
}

int main(int argc,char *argv[])
{
    string provider("pva");
    shared_vector<string> channelNames;
    channelNames.push_back("PVRbyte");
    channelNames.push_back("PVRshort");
    channelNames.push_back("PVRint");
    channelNames.push_back("PVRlong");
    channelNames.push_back("PVRubyte");
    channelNames.push_back("PVRushort");
    channelNames.push_back("PVRuint");
    channelNames.push_back("PVRulong");
    channelNames.push_back("PVRfloat");
    channelNames.push_back("PVRdouble");
    int opt;
    while((opt = getopt(argc, argv, "hp:")) != -1) {
        switch(opt) {
            case 'p':
                provider = optarg;
                break;
            case 'h':
             cout << " -h -p provider channelNames " << endl;
             cout << "default" << endl;
             cout << "-p " << provider 
                  << " " <<  channelNames
                  << endl;           
                return 0;
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
    cout << "_____pvaClientPutGet starting_______\n";

    try {
        int nPvs = argc - optind;       /* Remaining arg list are PV names */
        if (nPvs!=0)
        {
            channelNames.clear();
            while(optind < argc) {
                channelNames.push_back(argv[optind]);
                optind++;
            }
        }
        cout << " channelNames " <<  channelNames << endl;
        shared_vector<const string> names(freeze(channelNames));
        example(provider,names);
        cout << "_____pvaClientPutGet done_______\n";
    } catch (std::exception& e) {
        cout << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
