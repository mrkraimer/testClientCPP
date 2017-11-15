/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author mrk
 */

/* Author: Marty Kraimer */
#include <iostream>
#include <stdlib.h> 
#include <pv/pvaClient.h>
#include <pv/convert.h>

using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaClient;


int main(int argc,char *argv[])
{
    int ntimes(10);
    string providerName("pva");
    string channelName("DBRdoubleArray");
    string request("value");
    bool debug(false);
    if(argc==2 && string(argv[1])==string("-help")) {
        cout << "ntimes providerName channelName request debug" << endl;
        cout << "default" << endl;
        cout << ntimes << " "
             << providerName << " " <<  channelName
             << " " << '"' << request << '"'
             << " " << (debug ? "true" : "false") << endl;
        return 0;
    }
    if(argc>1) ntimes = atoi(argv[1]);
    if(argc>2) providerName = argv[2];
    if(argc>3) channelName = argv[3];
    if(argc>4) request = argv[4];
    if(argc>5) {
        string value(argv[5]);
        if(value=="true") debug = true;
    }
    cout << "ntimes " << ntimes
         << " providerName " << providerName
         << " channelName " <<  channelName
         << " request " << request
         << " debug " << (debug ? "true" : "false") << endl;

    cout << "_____putMany starting__\n";
    
    try {   
        if(debug) PvaClient::setDebug(true);
        PvaClientPtr pva= PvaClient::get(providerName);
        while(true) {
            cout << "Type exit to stop: \n";
            int c = std::cin.peek();  // peek character
            if ( c == EOF ) continue;
            string str;
            getline(cin,str);
            if(str.compare("exit")==0) break;
            PvaClientChannelPtr pvaChannel = pva->createChannel(channelName,providerName);
            pvaChannel->issueConnect();
            Status status = pvaChannel->waitConnect();
            if(!status.isOK()) {
                cout << "channel not conected\n";
                continue;
            }
            double nextvalue = 0.0;
            for(int i=0; i< ntimes; ++i) {
                PvaClientPutPtr pvaClientPut = pvaChannel->createPut();
                pvaClientPut->issueConnect();
                Status status = pvaClientPut->waitConnect();
                if(!status.isOK()) {
                    cout << "channelPut not conected\n";
                    continue;
                }
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
