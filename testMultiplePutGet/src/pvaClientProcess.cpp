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
#include <pv/pvaClient.h>
#include <pv/convert.h>

using std::tr1::static_pointer_cast;
using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaClient;

static void example(
     PvaClientPtr const &pvaClient,
     string provider,
     shared_vector<const string> const &channelNames)
{
    size_t num = channelNames.size();
    shared_vector<PvaClientChannelPtr> pvaClientChannels(num);
    shared_vector<PvaClientProcessPtr> pvaClientProcesss(num);
    for(size_t i=0; i<num; i++) {
        pvaClientChannels[i] = pvaClient->createChannel(channelNames[i],provider);
        pvaClientChannels[i]->issueConnect();
    }
    for(size_t i=0; i<num; i++) {
        Status status = pvaClientChannels[i]->waitConnect(5.0);
        if(!status.isOK()) {
            cout << "channel=" << channelNames[i] << " connect failed " << status <<"\n";
            return;
        }
        pvaClientProcesss[i] = pvaClientChannels[i]->createProcess();
        pvaClientProcesss[i]->issueConnect();
    }
    for(size_t i=0; i<num; i++) {
        Status status = pvaClientProcesss[i]->waitConnect();
        if(!status.isOK()) {
            cout << "channel=" << channelNames[i] << " put connect failed " << status <<"\n";
            return;
        }
    }
    int successCount = 0;
    int failedCount = 0;
    clock_t startTime = clock();
    int numiter = 10000;
    for(int i = 0; i< numiter; i+= 1) {
        bool correctData = true;
        for(size_t j=0; j<num; j++) {
            try {
                pvaClientProcesss[j]->issueProcess();
             } catch (std::exception& e) {
                    cout << " issueProcess exception " << e.what() << endl;
                 correctData = false;    
             }
        }
        for(size_t j=0; j<num; j++) {
            try {
                Status status = pvaClientProcesss[j]->waitProcess();
                if(!status.isOK()) {
                    cout << "i=" << i << " channel=" << channelNames[j]
                        << " waitProcess failed " << status <<"\n";
                    correctData = false;
                }
            } catch (std::exception& e) {
                 cout << "i=" << i << " waitPut exception " << e.what() << endl;
                 correctData = false;
            }
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
    cout << "FAILED COUNT: " << failedCount << endl;
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
        PvaClientPtr pva= PvaClient::get(provider);
        shared_vector<const string> names(freeze(channelNames));
        example(pva,provider,names);
        cout << "_____pvaClientPutGet done_______\n";
    } catch (std::exception& e) {
        cout << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
