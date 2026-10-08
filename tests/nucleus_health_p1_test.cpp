#include "trinity/nucleus_health.hpp"
#include "trinity/platform.hpp"
#include "trinity/serial.hpp"
#include <cstdio>
#include <cstdlib>

extern "C" trinity::u64 trinity_rdtsc(){ return 1u; }
namespace trinity::platform { u64 tsc_hz(){ return 1000u; } }
namespace trinity::serial {
void init(){} void putc(char){} void write(const char*){} void hex(u64){} void dec(u64){} void line(const char*){}
}

static void require(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}

int main(){
    using namespace trinity::nucleus;
    health_initialize();
    require(health_register(SubsystemId::Kernel,1000,3000,HealthCritical|HealthRequireProgress),"register kernel");
    require(health_register(SubsystemId::Compositor,1000,3000,0),"register compositor");

    health_heartbeat_at(SubsystemId::Kernel,1,100);
    health_heartbeat_at(SubsystemId::Compositor,7,100);
    auto s=health_snapshot();
    require(s.records[0].state==static_cast<trinity::u8>(HealthState::Healthy),"kernel healthy");

    // A live heartbeat with no forward progress must not conceal a stalled
    // progress-required subsystem.
    health_heartbeat_at(SubsystemId::Kernel,1,1500);
    health_poll_at(2200,1000);
    s=health_snapshot();
    require(s.records[0].state==static_cast<trinity::u8>(HealthState::Suspect),"kernel suspect on no progress");

    health_poll_at(3300,1000);
    s=health_snapshot();
    require(s.records[0].state==static_cast<trinity::u8>(HealthState::Stalled),"kernel stalled on no progress");
    require(s.critical_stalled==1u,"critical stalled count");

    health_heartbeat_at(SubsystemId::Kernel,2,3400);
    s=health_snapshot();
    require(s.records[0].state==static_cast<trinity::u8>(HealthState::Healthy),"kernel recovery");
    require(s.records[0].recoveries==1u,"recovery count");

    // Event-driven subsystem: same token is okay if heartbeat itself remains live.
    health_heartbeat_at(SubsystemId::Compositor,7,2500);
    health_poll_at(3200,1000);
    s=health_snapshot();
    require(s.records[static_cast<unsigned>(SubsystemId::Compositor)].state==static_cast<trinity::u8>(HealthState::Healthy),"compositor heartbeat-only healthy");

    std::puts("NUCLEUS HEALTH P1 PASS");
    return 0;
}
