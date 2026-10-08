#include "trinity/nucleus_health.hpp"
#include "trinity/platform.hpp"
#include "trinity/serial.hpp"
#include "trinity/arch.hpp"
#include <atomic>

namespace trinity::nucleus {
namespace {
struct Entry {
    std::atomic<u8> registered{0};
    std::atomic<u8> state{static_cast<u8>(HealthState::Offline)};
    std::atomic<u8> flags{0};
    std::atomic<u32> soft_timeout_ms{0};
    std::atomic<u32> hard_timeout_ms{0};
    std::atomic<u64> heartbeat_count{0};
    std::atomic<u64> progress_token{0};
    std::atomic<u64> last_heartbeat_tsc{0};
    std::atomic<u64> last_progress_tsc{0};
    std::atomic<u64> state_changes{0};
    std::atomic<u64> recoveries{0};
};

Entry g_entries[static_cast<usize>(SubsystemId::Count)]{};

constexpr usize index_of(SubsystemId id) { return static_cast<usize>(id); }

const char* name_of(SubsystemId id) {
    switch (id) {
        case SubsystemId::Kernel: return "KERNEL";
        case SubsystemId::Scheduler: return "SCHEDULER";
        case SubsystemId::Desktop: return "DESKTOP";
        case SubsystemId::Compositor: return "COMPOSITOR";
        case SubsystemId::Input: return "INPUT";
        case SubsystemId::Filesystem: return "FILESYSTEM";
        case SubsystemId::Network: return "NETWORK";
        case SubsystemId::Audio: return "AUDIO";
        case SubsystemId::Gpu: return "GPU";
        case SubsystemId::Storage: return "STORAGE";
        default: return "UNKNOWN";
    }
}

u64 elapsed_ms(u64 then, u64 now, u64 hz) {
    if (!then || now < then || !hz) return 0;
    const u64 delta = now - then;
    const u64 seconds = delta / hz;
    const u64 remainder = delta % hz;
    if (seconds > (~0ull / 1000ull)) return ~0ull;
    return seconds * 1000ull + (remainder * 1000ull) / hz;
}

void publish_state(SubsystemId id, Entry& e, HealthState next) {
    const u8 desired = static_cast<u8>(next);
    const u8 prior = e.state.exchange(desired, std::memory_order_acq_rel);
    if (prior == desired) return;
    e.state_changes.fetch_add(1u, std::memory_order_relaxed);
    if ((prior == static_cast<u8>(HealthState::Suspect) ||
         prior == static_cast<u8>(HealthState::Stalled)) &&
        next == HealthState::Healthy) {
        e.recoveries.fetch_add(1u, std::memory_order_relaxed);
    }
    serial::write("[NUCLEUS:P1] ");
    serial::write(name_of(id));
    serial::write(" health ");
    serial::dec(prior);
    serial::write(" -> ");
    serial::dec(desired);
    serial::putc('\n');
}
} // namespace

void health_initialize() {
    for (usize i = 0; i < static_cast<usize>(SubsystemId::Count); ++i) {
        Entry& e = g_entries[i];
        e.registered.store(0, std::memory_order_relaxed);
        e.state.store(static_cast<u8>(HealthState::Offline), std::memory_order_relaxed);
        e.flags.store(0, std::memory_order_relaxed);
        e.soft_timeout_ms.store(0, std::memory_order_relaxed);
        e.hard_timeout_ms.store(0, std::memory_order_relaxed);
        e.heartbeat_count.store(0, std::memory_order_relaxed);
        e.progress_token.store(0, std::memory_order_relaxed);
        e.last_heartbeat_tsc.store(0, std::memory_order_relaxed);
        e.last_progress_tsc.store(0, std::memory_order_relaxed);
        e.state_changes.store(0, std::memory_order_relaxed);
        e.recoveries.store(0, std::memory_order_relaxed);
    }
}

bool health_register(SubsystemId id, u32 soft_timeout_ms, u32 hard_timeout_ms, u8 flags) {
    const usize index = index_of(id);
    if (index >= static_cast<usize>(SubsystemId::Count) || !soft_timeout_ms ||
        hard_timeout_ms < soft_timeout_ms) return false;
    Entry& e = g_entries[index];
    const u64 now = trinity_rdtsc();
    e.flags.store(flags, std::memory_order_relaxed);
    e.soft_timeout_ms.store(soft_timeout_ms, std::memory_order_relaxed);
    e.hard_timeout_ms.store(hard_timeout_ms, std::memory_order_relaxed);
    e.last_heartbeat_tsc.store(now, std::memory_order_relaxed);
    e.last_progress_tsc.store(now, std::memory_order_relaxed);
    e.state.store(static_cast<u8>(HealthState::Starting), std::memory_order_release);
    e.registered.store(1u, std::memory_order_release);
    return true;
}

void health_heartbeat_at(SubsystemId id, u64 progress_token, u64 now_tsc) {
    const usize index = index_of(id);
    if (index >= static_cast<usize>(SubsystemId::Count)) return;
    Entry& e = g_entries[index];
    if (!e.registered.load(std::memory_order_acquire)) return;
    e.heartbeat_count.fetch_add(1u, std::memory_order_relaxed);
    e.last_heartbeat_tsc.store(now_tsc, std::memory_order_release);
    const u64 prior = e.progress_token.exchange(progress_token, std::memory_order_acq_rel);
    if (progress_token != prior || e.last_progress_tsc.load(std::memory_order_acquire) == 0u)
        e.last_progress_tsc.store(now_tsc, std::memory_order_release);
    publish_state(id, e, HealthState::Healthy);
}

void health_heartbeat(SubsystemId id, u64 progress_token) {
    health_heartbeat_at(id, progress_token, trinity_rdtsc());
}

void health_poll_at(u64 now_tsc, u64 tsc_hz) {
    if (!tsc_hz) return;
    for (usize i = 0; i < static_cast<usize>(SubsystemId::Count); ++i) {
        Entry& e = g_entries[i];
        if (!e.registered.load(std::memory_order_acquire)) continue;
        const u8 flags = e.flags.load(std::memory_order_relaxed);
        const u64 last = (flags & HealthRequireProgress)
            ? e.last_progress_tsc.load(std::memory_order_acquire)
            : e.last_heartbeat_tsc.load(std::memory_order_acquire);
        if (!last) continue;
        const u64 age = elapsed_ms(last, now_tsc, tsc_hz);
        const u32 hard = e.hard_timeout_ms.load(std::memory_order_relaxed);
        const u32 soft = e.soft_timeout_ms.load(std::memory_order_relaxed);
        const auto id = static_cast<SubsystemId>(i);
        if (age >= hard) publish_state(id, e, HealthState::Stalled);
        else if (age >= soft) publish_state(id, e, HealthState::Suspect);
    }
}

void health_poll() { health_poll_at(trinity_rdtsc(), platform::tsc_hz()); }

HealthSnapshot health_snapshot() {
    HealthSnapshot out{};
    out.count = static_cast<u32>(SubsystemId::Count);
    out.sample_tsc = trinity_rdtsc();
    for (usize i = 0; i < static_cast<usize>(SubsystemId::Count); ++i) {
        const Entry& e = g_entries[i];
        HealthRecord& r = out.records[i];
        r.id = static_cast<u8>(i);
        r.registered = e.registered.load(std::memory_order_acquire);
        r.state = e.state.load(std::memory_order_acquire);
        r.flags = e.flags.load(std::memory_order_relaxed);
        r.soft_timeout_ms = e.soft_timeout_ms.load(std::memory_order_relaxed);
        r.hard_timeout_ms = e.hard_timeout_ms.load(std::memory_order_relaxed);
        r.heartbeat_count = e.heartbeat_count.load(std::memory_order_relaxed);
        r.progress_token = e.progress_token.load(std::memory_order_relaxed);
        r.last_heartbeat_tsc = e.last_heartbeat_tsc.load(std::memory_order_acquire);
        r.last_progress_tsc = e.last_progress_tsc.load(std::memory_order_acquire);
        r.state_changes = e.state_changes.load(std::memory_order_relaxed);
        r.recoveries = e.recoveries.load(std::memory_order_relaxed);
        if (!r.registered) continue;
        if (r.state == static_cast<u8>(HealthState::Healthy)) ++out.healthy;
        else if (r.state == static_cast<u8>(HealthState::Suspect)) ++out.suspect;
        else if (r.state == static_cast<u8>(HealthState::Stalled)) {
            ++out.stalled;
            if (r.flags & HealthCritical) ++out.critical_stalled;
        }
    }
    return out;
}

void health_register_pass1_defaults() {
    // Timeouts are intentionally conservative for first hardware qualification.
    // R1 detects only; Pass 2 may attach bounded recovery actions after these
    // signals have been proven trustworthy on hardware.
    (void)health_register(SubsystemId::Kernel, 2000u, 10000u,
        static_cast<u8>(HealthCritical | HealthRequireProgress));
    (void)health_register(SubsystemId::Scheduler, 1000u, 5000u,
        static_cast<u8>(HealthCritical | HealthRequireProgress));
    (void)health_register(SubsystemId::Desktop, 2000u, 10000u,
        static_cast<u8>(HealthCritical | HealthRequireProgress));
    (void)health_register(SubsystemId::Compositor, 2000u, 10000u, HealthCritical);
    (void)health_register(SubsystemId::Input, 2000u, 10000u, HealthCritical);
}

} // namespace trinity::nucleus
