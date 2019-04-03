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
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/convert.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>

using namespace std;
using namespace epics::pvData;
using std::tr1::static_pointer_cast;

static size_t getsize(PVFieldPtr const & pvField)
{
    if(pvField->getField()->getType()==scalar) {
        size_t s = sizeof(pvField.get());
//cout << pvField->getFieldName() << " nbytes " << s << "\n";
        return s;
    }
    PVStructurePtr pvStructure = static_pointer_cast<PVStructure>(pvField);
    PVFieldPtrArray pvFields = pvStructure->getPVFields();
    size_t nbytes = 0;
    for(size_t i=0; i<pvFields.size(); ++i) {
        nbytes += getsize(pvFields[i]);
    }
    return nbytes;
}

static size_t getnumberfields(PVFieldPtr const & pvField)
{
    if(pvField->getField()->getType()==scalar) {
        size_t s = 1;
//cout << pvField->getFieldName() << " nbytes " << s << "\n";
        return s;
    }
    PVStructurePtr pvStructure = static_pointer_cast<PVStructure>(pvField);
    PVFieldPtrArray pvFields = pvStructure->getPVFields();
    size_t nfields = pvFields.size();
    for(size_t i=0; i<pvFields.size(); ++i) {
        nfields += getnumberfields(pvFields[i]);
    }
    return nfields;
}


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
    StandardPVFieldPtr standardPVField = getStandardPVField();
    try {   
        PVStructurePtr pvStructure(standardPVField->scalar(
           pvDouble,request));
        size_t nbytes = getsize(pvStructure);
        cout << "total data bytes " << nbytes << "\n";
        size_t nfields = getnumberfields(pvStructure);
        cout << "total fields " << nfields  << "\n";

    } catch(std::exception& e) {
        cerr << "exception " << e.what() << endl;
        return 1;
    }
    return 0;
}
