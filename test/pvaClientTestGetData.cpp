// Copyright information and license terms for this software can be
// found in the file LICENSE that is included with the distribution

/*pvaClientTestGetData.cpp */

/**
 * @author mrk
 */

/* Author: Marty Kraimer */

#include <iostream>

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/pvaClient.h>
#include <pv/bitSet.h>

using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaClient;

static PvaClientPtr pvaClient;
static FieldCreatePtr fieldCreate;
static StandardFieldPtr standardField;
static PVDataCreatePtr pvDataCreate;

static void testDouble()
{
    testDiag("== testDouble ==");
    StructureConstPtr structure =
       fieldCreate->createFieldBuilder() ->
            add("alarm",standardField->alarm()) ->
            add("timeStamp",standardField->timeStamp()) ->
            add("value",pvDouble) ->
            createStructure();

    PvaClientGetDataPtr pvaData = PvaClientGetData::create(structure);
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(pvaData->getStructure());

    BitSetPtr bitSet = BitSetPtr(new BitSet(pvStructure->getNumberFields()));
    pvaData->setData(pvStructure,bitSet);

    PVDoublePtr pvDouble = pvStructure->getSubField<PVDouble>("value");
    size_t valueOffset = pvDouble->getFieldOffset();

    BitSetPtr change = pvaData->getChangedBitSet();
    pvDouble->put(5.0);
    change->set(pvDouble->getFieldOffset());

    testOk(change->cardinality()==1,"1 field changed");
    testOk(change->get(valueOffset),"value changed");

    testOk(pvaData->hasValue(),"hasValue");
    testOk(pvaData->isValueScalar(),"isValueScalar");
    testOk(!pvaData->isValueScalarArray(),"!isValueScalarArray");

    try {
        testOk(!!pvaData->getValue(), "getValue");
    } catch (std::exception& e) {
        testFail("getValue exception '%s'", e.what());
    }

    try {
        testOk(!!pvaData->getScalarValue(), "getScalarValue");
    } catch (std::exception& e) {
        testFail("getScalarValue exception '%s'", e.what());
    }
    try {
        testOk(!pvaData->getArrayValue(), "!getArrayValue");
    } catch (std::exception& e) {
        testPass("getArrayValue exception '%s'", e.what());
    }
    try {
        testOk(!pvaData->getScalarArrayValue(), "!getScalarArrayValue");
    } catch (std::exception& e) {
        testPass("getScalarArrayValue exception '%s'", e.what());
    }

    try {
        testOk(pvaData->getDouble() == 5.0, "getDouble value");
    } catch (std::exception& e) {
        testFail("getDouble exception '%s'", e.what());
    }
    try {
        testOk(pvaData->getString() == "5", "getString value");
    } catch (std::exception& e) {
        testFail("getString exception '%s'", e.what());
    }

    try {
        shared_vector<const double> value = pvaData->getDoubleArray();
        testFail("getDoubleArray");
    } catch (std::exception& e) {
        testPass("getDoubleArray exception '%s'", e.what());
    }
    try {
        shared_vector<const string> value = pvaData->getStringArray();
        testFail("getStringArray");
    } catch (std::exception& e) {
        testPass("getStringArray exception '%s'", e.what());
    }
}

static void testDoubleArray()
{
    testDiag("== testDoubleArray ==");
    StructureConstPtr structure =
       fieldCreate->createFieldBuilder() ->
            add("alarm",standardField->alarm()) ->
            add("timeStamp",standardField->timeStamp()) ->
            addArray("value",pvDouble) ->
            createStructure();

    PvaClientGetDataPtr pvaData = PvaClientGetData::create(structure);
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(pvaData->getStructure());

    BitSetPtr bitSet = BitSetPtr(new BitSet(pvStructure->getNumberFields()));
    pvaData->setData(pvStructure,bitSet);

    PVDoubleArrayPtr pvalue = pvaData->getPVStructure() ->
        getSubField<PVDoubleArray>("value");

    BitSetPtr change = pvaData->getChangedBitSet();
    size_t valueOffset = pvalue->getFieldOffset();

    size_t len = 5;
    shared_vector<double> value(len);
    for (size_t i=0; i<len; ++i)
        value[i] = i*10.0;
    pvalue->replace(freeze(value));
    change->set(valueOffset);

    testOk(change->cardinality()==1,"1 field changed");
    testOk(change->get(valueOffset),"value changed");

    testOk(pvaData->hasValue(),"hasValue");
    testOk(!pvaData->isValueScalar(),"!isValueScalar");
    testOk(pvaData->isValueScalarArray(),"isValueScalarArray");

    try {
        testOk(!!pvaData->getValue(), "getValue");
    } catch (std::exception& e) {
        testFail("getValue exception '%s'", e.what());
    }

    try {
        testOk(!pvaData->getScalarValue(), "!getScalarValue");
    } catch (std::exception& e) {
        testPass("getScalarValue exception '%s'", e.what());
    }
    try {
        testOk(!!pvaData->getArrayValue(), "getArrayValue");
    } catch (std::exception& e) {
        testFail("getArrayValue exception '%s'", e.what());
    }
    try {
        testOk(!!pvaData->getScalarArrayValue(), "getScalarArrayValue");
    } catch (std::exception& e) {
        testPass("getScalarArrayValue exception '%s'", e.what());
    }

    try {
        testFail("getDouble %g", pvaData->getDouble());
    } catch (std::exception& e) {
        testPass("getDouble exception '%s'", e.what());
    }
    try {
        testFail("getString %s", pvaData->getString().c_str());
    } catch (std::exception& e) {
        testPass("getString exception '%s'", e.what());
    }

    try {
        shared_vector<const double> value = pvaData->getDoubleArray();
        testPass("getDoubleArray");
    } catch (std::exception& e) {
        testFail("getDoubleArray exception '%s'", e.what());
    }
    try {
        shared_vector<const string> value = pvaData->getStringArray();
        testPass("getStringArray");
    } catch (std::exception& e) {
        testFail("getStringArray exception '%s'", e.what());
    }
}

MAIN(pvaClientTestGetData)
{
    pvaClient = PvaClient::get("pva");
    fieldCreate = getFieldCreate();
    standardField = getStandardField();
    pvDataCreate = getPVDataCreate();

    testPlan(26);
    testDiag("=== pvaClientTestGetData ===");

    testDouble();
    testDoubleArray();
    testDone();
pvaClient.reset();
fieldCreate.reset();
standardField.reset();
pvDataCreate.reset();
    return 0;
}

