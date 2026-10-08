#pragma once
#include "base.hpp"

namespace trinity::nucleus {

enum class SubsystemId : u8 {
    Kernel = 0,
    Scheduler,
    Desktop,
    Compositor,
    Input,
    Filesystem,
    Network,
    Audio,
    Gpu,
    Storage,
    Count,
};

enum class HealthState : u8 {
    Offline = 0,
    Starting,
    Healthy,
    Suspect,
    Stalled,
};

enum HealthFlags : u8 {
    HealthCritical = 1u << 0,
    HealthRequireProgress = 1u << 1,
};

struct HealthRecord {
    u8 id{0};
    u8 state{0};
    u8 flags{0};
    u8 registered{0};
    u32 soft_timeout_ms{0};
    u32 hard_timeout_ms{0};
    u64 heartbeat_count{0};
    u64 progress_token{0};
    u64 last_heartbeat_tsc{0};
    u64 last_progress_tsc{0};
    u64 state_changes{0};
    u64 recoveries{0};
};

struct HealthSnapshot {
    static constexpr usize Capacity = static_cast<usize>(SubsystemId::Count);
    u32 version{1};
    u32 count{0};
    u32 healthy{0};
    u32 suspect{0};
    u32 stalled{0};
    u32 critical_stalled{0};
    u64 sample_tsc{0};
    HealthRecord records[Capacity]{};
};

// Pass 1 is detection/telemetry only. No recovery action is performed here.
void health_initialize();
bool health_register(SubsystemId id, u32 soft_timeout_ms, u32 hard_timeout_ms, u8 flags);
#if defined(TRINITY_HOST_TEST)
// Legacy GUI host tests include gui_runtime.cpp directly and intentionally do
// not link the Nucleus implementation. Keep those tests focused on their
// subsystem while the dedicated Nucleus P1 test exercises the real detector.
inline void health_heartbeat(SubsystemId, u64) {}
inline void health_poll() {}
#else
void health_heartbeat(SubsystemId id, u64 progress_token);
void health_poll();
#endif
void health_heartbeat_at(SubsystemId id, u64 progress_token, u64 now_tsc);
void health_poll_at(u64 now_tsc, u64 tsc_hz);
HealthSnapshot health_snapshot();

// Register the initially authoritative producers. Other domains remain Offline
// until their own subsystem integration pass begins publishing heartbeats.
void health_register_pass1_defaults();

} // namespace trinity::nucleus
