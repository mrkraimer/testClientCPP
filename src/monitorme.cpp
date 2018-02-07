/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvAccessCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#include <set>
#include <queue>
#include <vector>
#include <string>
#include <exception>

#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsGuard.h>
#include <epicsGetopt.h>

#include <pv/configuration.h>
#include <pv/caProvider.h>
#include <pv/reftrack.h>
#include <pv/thread.h>
#include <pva/client.h>
#include <pv/timeStamp.h>

using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

namespace {

typedef epicsGuard<epicsMutex> Guard;
typedef epicsGuardRelease<epicsMutex> UnGuard;

struct MonTracker : public pvac::ClientChannel::MonitorCallback,
                    public std::tr1::enable_shared_from_this<MonTracker>
{
    POINTER_DEFINITIONS(MonTracker);

    MonTracker(const std::string& name)
   :name(name),
    numMonitor(0)
    {
         timeStampLast.getCurrent();
    }
    virtual ~MonTracker() {mon.cancel();}

    const std::string name;
    pvac::Monitor mon;
    volatile long numMonitor;
    TimeStamp timeStamp;
    TimeStamp timeStampLast;

    virtual void monitorEvent(const pvac::MonitorEvent& evt) OVERRIDE FINAL
    {
        // shared_from_this() will fail as Cancel is delivered in our dtor.
        if(evt.event==pvac::MonitorEvent::Cancel) return;

        // running on internal provider worker thread
        // minimize work here.
        // TODO: bound queue size
       // monwork.push(shared_from_this(), evt);
        switch(evt.event) {
            case pvac::MonitorEvent::Fail:
                std::cout<<"Error "<<name<<" "<<evt.message<<"\n";
                break;
            case pvac::MonitorEvent::Cancel:
                std::cout<<"Cancel "<<name<<"\n";
                break;
            case pvac::MonitorEvent::Disconnect:
                std::cout<<"Disconnect "<<name<<"\n";
                break;
            case pvac::MonitorEvent::Data:
            {
                unsigned n;
                for(n=0; n<2 && mon.poll(); n++)
                {
                    pvd::PVField::const_shared_pointer fld(mon.root->getSubField("value"));
                    if(!fld)
                        fld = mon.root;

                    std::cout<<"Event "<<name<<" "<<fld
                        <<" Changed:"<<mon.changed
                        <<" overrun:"<<mon.overrun<<"\n";
                }
            }
        }
    }

};

} // namespace

int main(int argc, char *argv[]) {
    try {
        epics::RefMonitor refmon;
        std::string providerName("pva"),
                    requestStr("field()");
        typedef std::vector<std::string> pvs_t;
        pvs_t pvs;

        int opt;
        while((opt = getopt(argc, argv, "hRp:w:r:")) != -1) {
            switch(opt) {
            case 'R':
                refmon.start(5.0);
                break;
            case 'p':
                providerName = optarg;
                break;
            case 'r':
                requestStr = optarg;
                break;
            case 'h':
                std::cout<<"Usage: "<<argv[0]<<" [-p <provider>] [-w <timeout>] [-r <request>] [-R] <pvname> ...\n";
                return 0;
            default:
                std::cerr<<"Unknown argument: "<<opt<<"\n";
                return -1;
            }
        }

        for(int i=optind; i<argc; i++)
            pvs.push_back(argv[i]);

        // build "pvRequest" which asks for all fields
        pvd::PVStructure::shared_pointer pvReq(pvd::createRequest(requestStr));

        // explicitly select configuration from process environment
        pva::Configuration::shared_pointer conf(pva::ConfigurationBuilder()
                                                .push_env()
                                                .build());

        // "pva" provider automatically in registry
        // add "ca" provider to registry
        pva::ca::CAClientFactory::start();

        std::cout<<"Use provider: "<<providerName<<"\n";
        pvac::ClientProvider provider(providerName, conf);

        std::vector<MonTracker::shared_pointer> monitors;

        for(pvs_t::const_iterator it=pvs.begin(); it!=pvs.end(); ++it) {
            const std::string& pv = *it;

            MonTracker::shared_pointer mon(new MonTracker(pv));

            pvac::ClientChannel chan(provider.connect(pv));

            mon->mon = chan.monitor(mon.get(), pvReq);

            monitors.push_back(mon);
        }
        int ret = 0;
        string str;
        getline(cin,str);
cout << "after getline\n";
        if(refmon.running()) {
            refmon.stop();
            // drop refs to operations, but keep ref to ClientProvider
            monitors.clear();
            // show final counts
            refmon.current();
        }
cout << "calling monitors.clear\n";
monitors.clear();
cout << "returning\n";
        return ret;
    } catch(std::exception& e){
        std::cout<<"Error: "<<e.what()<<"\n";
        return 2;
    }
}
