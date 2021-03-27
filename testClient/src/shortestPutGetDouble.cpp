/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author mrk
 */

#include <iostream>
#include <epicsGetopt.h>
#include <pv/pvaClient.h>


using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaClient;

int main(int argc,char *argv[])
{
   string channelName("PVRdouble");
   int nPvs = argc - optind;
   if(nPvs==1) channelName = argv[optind];
   try {
      double value = 0;
      while(true) {
          cout << "Type exit to stop: \n";
          int c = std::cin.peek();  // peek character
          if ( c == EOF ) continue;
          string str;
          getline(cin,str);
          if(str.compare("exit")==0) break;
          cout << PvaClient::get("pva")->channel(channelName)->getDouble() << "\n";
          PvaClient::get("pva")->channel(channelName)->putDouble(value);
          value = value + 1;
        }
    } catch (std::exception& e) {
        cerr << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
