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

struct Worker {
    virtual ~Worker() {}
    virtual void process(const pvac::MonitorEvent& event) =0;
};

// simple work queue with thread.
// moves monitor queue handling off of PVA thread(s)
struct WorkQueue : public epicsThreadRunable {
    epicsMutex mutex;
    typedef std::tr1::shared_ptr<Worker> value_type;
    typedef std::tr1::weak_ptr<Worker> weak_type;
    // work queue holds only weak_ptr
    // so jobs must be kept alive seperately
    typedef std::deque<std::pair<weak_type, pvac::MonitorEvent> > queue_t;
    queue_t queue;
    epicsEvent event;
    bool running;
    pvd::Thread worker;

    WorkQueue()
        :running(true)
        ,worker(pvd::Thread::Config()
                .name("Monitor handler")
                .autostart(true)
                .run(this))
    {}
    ~WorkQueue() {close();}

    void close()
    {
        {
            Guard G(mutex);
            running = false;
        }
        event.signal();
        worker.exitWait();
    }

    void push(const weak_type& cb, const pvac::MonitorEvent& evt)
    {
        bool wake;
        {
            Guard G(mutex);
            if(!running) return; // silently refuse to queue during/after close()
            wake = queue.empty();
            queue.push_back(std::make_pair(cb, evt));
        }
        if(wake)
            event.signal();
    }

    virtual void run() OVERRIDE FINAL
    {
        Guard G(mutex);

        while(running) {
            if(queue.empty()) {
                UnGuard U(G);
                event.wait();
            } else {
                queue_t::value_type ent(queue.front());
                value_type cb(ent.first.lock());
                queue.pop_front();
                if(!cb) continue;

                try {
                    UnGuard U(G);
                    cb->process(ent.second);
                }catch(std::exception& e){
                    std::cout<<"Error in monitor handler : "<<e.what()<<"\n";
                }
            }
        }
    }
};

WorkQueue monwork;

struct MonTracker : public pvac::ClientChannel::MonitorCallback,
                    public Worker,
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
    epicsMutex mutex;

    virtual void monitorEvent(const pvac::MonitorEvent& evt) OVERRIDE FINAL
    {
        // shared_from_this() will fail as Cancel is delivered in our dtor.
        if(evt.event==pvac::MonitorEvent::Cancel) return;

        // running on internal provider worker thread
        // minimize work here.
        // TODO: bound queue size
        monwork.push(shared_from_this(), evt);
    }

    virtual void process(const pvac::MonitorEvent& evt) OVERRIDE FINAL
    {
        // running on our worker thread
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
            for(n=0; n<2 && mon.poll(); n++) {
            {
                Guard G(mutex);
                numMonitor++;
            }
            }
            if(n==2) {
                // too many updates, re-queue to balance with others
                monwork.push(shared_from_this(), evt);
            }
        }
            break;
        }
    }

    double report()
    {
        timeStamp.getCurrent();
        double  diff = TimeStamp::diff(timeStamp,timeStampLast);
        long numnow = 0;
        {
            Guard G(mutex);
            numnow = numMonitor;
            numMonitor = 0;
        }
        timeStampLast.getCurrent();
        double persecond = 0;
        if(diff>0.0) persecond = numnow/diff;
        cout << name
        << " numnow " << numnow
        << " persecond " << persecond
        << endl;
        return persecond;
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
        while(true) {
            string str;
            getline(cin,str);
            if(str.compare("exit")==0){
                 break;
            }
            double events = 0.0;
            for(size_t i=0; i<monitors.size(); ++i) {
                events += monitors[i]->report();
            }
            cout << "total events/second " << events << endl;
        }
        if(refmon.running()) {
            refmon.stop();
            // drop refs to operations, but keep ref to ClientProvider
            monitors.clear();
            // show final counts
            refmon.current();
        }
cout << "calling monwork.close\n";
        monwork.close();
cout << "calling monitors.clear\n";
monitors.clear();
cout << "returning\n";
        return ret;
    } catch(std::exception& e){
        std::cout<<"Error: "<<e.what()<<"\n";
        return 2;
    }
}
