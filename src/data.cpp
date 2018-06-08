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
#include <pv/pvData.h>
#include <pv/createRequest.h>

using namespace std;
using namespace epics::pvData;

class Data;
typedef std::tr1::shared_ptr<Data> DataPtr;

class Data :
    public std::tr1::enable_shared_from_this<Data>
{
private:
    PVStructurePtr pvStructure;
    string request;
    CreateRequest::shared_pointer createRequest;
public:
    POINTER_DEFINITIONS(Data);
    Data(const string &request)
    : request(request),
      createRequest(CreateRequest::create())
    {
    }

    ~Data()
    {
        cout<< "~Data()\n";
    }
    
    static DataPtr create(const string  & request)
    {
        DataPtr client(DataPtr(
             new Data(request)));
        return client;
    }

    void show()
    {
        pvStructure = createRequest->createRequest(request);
        if(pvStructure) {
            cout << pvStructure << endl;
        } else  {
            cout << "error " << createRequest->getMessage() << endl;
        }
    }
    
};

int main(int argc,char *argv[])
{
    string request("value,alarm,timeStamp");
    int opt;
    while((opt = getopt(argc, argv, "hr:")) != -1) {
        switch(opt) {
            case 'r':
                request = optarg;
                break;
            case 'h':
             cout << " -h -r request " << endl;
             cout << "default" << endl;
             cout << " -r " << request
                  << endl;           
                return 0;
            default:
                std::cerr<<"Unknown argument: "<<opt<<"\n";
                return -1;
        }
    }
    cout << "_____data starting__\n";
    try {   
       DataPtr data(Data::create(request));
       data->show();
    } catch (std::runtime_error e) {
        cerr << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
