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

    cout << "_____putBlock starting__\n";
    
    try {   
        if(debug) PvaClient::setDebug(true);
        ConvertPtr convert = getConvert();
        PvaClientPtr pva= PvaClient::get(providerName);
        PvaClientChannelPtr pvaChannel = pva->createChannel(channelName,providerName);
        pvaChannel->issueConnect();
        PvaClientPutPtr pvaClientPut;
        double nextvalue = 0.0;
        while(true) {
            int c = std::cin.peek();  // peek character
            if ( c == EOF ) continue;
            cout << "Type exit to stop: \n";
            string str;
            getline(cin,str);
            if(str.compare("exit")==0) break;
            if(!pvaChannel->getChannel()->isConnected()) {
               cout << "not connected\n";
            } else if(!pvaClientPut) {
               cout << "creating pvaClientPut\n";
               pvaClientPut = pvaChannel->put();
            } else {
                 size_t n = 5;
                    shared_vector<double> value(n);
                    for(size_t i=0; i<n; ++i) {
                        value[i] = nextvalue;
                        nextvalue += 1.0;
                    }
                    pvaClientPut->getData()->putDoubleArray(freeze(value));
                    pvaClientPut->put();
                 
            }
        }
    } catch (std::runtime_error e) {
        cerr << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
