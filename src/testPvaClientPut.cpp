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

using std::tr1::static_pointer_cast;
using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaClient;

static ConvertPtr convert = getConvert();


static void exampleDouble(PvaClientPtr const &pva,string const & channelName,string const & providerName)
{
    cout << "__exampleDouble__ channelName " << channelName << " providerName " << providerName << endl;
    PvaClientChannelPtr channel = pva->channel(channelName,providerName,2.0);
    PvaClientPutPtr put = channel->put();
    PvaClientPutDataPtr putData = put->getData();
    PvaClientMonitorPtr monitor = pva->channel(channelName,providerName,2.0)->monitor("value");
    PvaClientMonitorDataPtr monitorData = monitor->getData();
    putData->putDouble(3.0); put->put();
    channel->get("field(value)")->getData()->showChanged(cout) << endl;
    putData->putDouble(4.0); put->put();
    channel->get("field(value)")->getData()->showChanged(cout) << endl;
    if(!monitor->waitEvent()) {
           cout << "waitEvent returned false. Why???";
    } else while(true) {
         cout << "monitor changed\n";
         monitorData->showChanged(cout);
         monitor->releaseEvent();
         if(!monitor->poll()) break;
    }
}

int main(int argc,char *argv[])
{
    cout << "_____examplePvaClientPut starting_______\n";
    try {
        PvaClientPtr pva = PvaClient::get("pva ca");
        for (int i=0; i<10; ++i)
        {
            exampleDouble(pva,"PVRdouble","pva");
        }
        cout << "_____examplePvaClientPut done_______\n";
    } catch (std::runtime_error e) {
        cerr << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
