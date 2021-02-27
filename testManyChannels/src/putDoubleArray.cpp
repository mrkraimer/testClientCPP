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


int main(int argc,char *argv[])
{
    string provider("pva");
    string channelName("PVRBigRecord");
    string request("scalarArray.double.value");
    try {   
        size_t nelements = 10;
        PvaClientPtr pva= PvaClient::get(provider);
        shared_vector<double> values(nelements,0);
        double value =0.0;
        for(size_t i=0; i< nelements ; ++i) values[i] = (value + i);
        pva->channel(channelName, provider, 1.0)->putDoubleArray(freeze(values), request);
        cout << pva->channel(channelName, provider, 1.0)->getDoubleArray(request);
    } catch(std::exception& e) {
        cerr << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
