/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author mrk
 */

/* Author: Marty Kraimer */

#include <iostream>
#include <epicsStdlib.h>
#include <epicsGetopt.h>
#include <epicsThread.h>
#include <pv/pvaClient.h>

using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaClient;

class MyMonitorRequester;
typedef std::tr1::shared_ptr<MyMonitorRequester> MyMonitorRequesterPtr;

class MyMonitorRequester:
     public PvaClientMonitorRequester
{
private:
    string name;
public:
    MyMonitorRequester(const string &name)
    {
        this->name = name;
    }
    
    virtual void event(PvaClientMonitorPtr const & monitor)
    {
        while(monitor->poll()) {
            PvaClientMonitorDataPtr pvaData = monitor->getData();
            PVStructurePtr pvStructure = pvaData->getPVStructure();
            cout << "name " << name << " pvStructure\n" << pvStructure << "\n";
            monitor->releaseEvent();
        }
    }
};


int main(int argc,char *argv[])
{
    string provider("pva");
    string channelName("PVRBigRecord");
    try { 
        PvaClientPtr pva= PvaClient::get("pva");
        PvaClientChannelPtr channel(pva->createChannel("PVRBigRecord"));
        channel->connect();
        MyMonitorRequesterPtr requestera(new MyMonitorRequester("a"));
        MyMonitorRequesterPtr requesterb(new MyMonitorRequester("b"));
        PvaClientMonitorPtr monitora = channel->monitor("scalar.double",requestera);
        PvaClientMonitorPtr monitorb = channel->monitor("scalarArray.double",requesterb);
        while(true) {
            string str;
            getline(cin,str);
            if(str.compare("exit")==0) break;
            cout << str << " not a legal commnd\n";
        }
    } catch(std::exception& e) {
        cerr << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
