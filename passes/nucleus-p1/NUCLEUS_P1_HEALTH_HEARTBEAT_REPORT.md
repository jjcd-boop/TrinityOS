# Trinity OS — Nucleus Pass 1: Health & Heartbeat Framework

## Scope
Pass 1 adds observation-only Nucleus health supervision. It does **not** restart, kill, quarantine, or fence any subsystem. Recovery authority is deferred to Pass 2 after hardware qualification of the detector.

## Implemented
- Fixed subsystem registry for Kernel, Scheduler, Desktop, Compositor, Input, Filesystem, Network, Audio, GPU, and Storage.
- Initial authoritative heartbeat producers: Kernel, Scheduler, Desktop, Compositor, Input.
- Forward-progress tokens for Kernel, Scheduler, and Desktop, so repeated check-ins without actual progress can still transition to Suspect/Stalled.
- Event-driven heartbeat mode for Compositor/Input so idle systems are not falsely declared stalled.
- States: Offline, Starting, Healthy, Suspect, Stalled.
- Separate soft and hard timeout thresholds.
- Critical-stall aggregate count.
- Transition and recovery counters.
- Dual bounded health evaluation paths: CPU0 scheduler timer and kernel syscall heartbeat.
- Read-only `NUCLEUS` console diagnostic snapshot via `GetNucleusHealth` syscall, limited to the Desktop/console extension.
- Deterministic host regression test and `scripts/verify_nucleus_p1.sh`.

## Initial timeout policy
- Kernel: suspect 2 s, stalled 10 s, critical + progress-required.
- Scheduler: suspect 1 s, stalled 5 s, critical + progress-required.
- Desktop: suspect 2 s, stalled 10 s, critical + progress-required.
- Compositor: suspect 2 s, stalled 10 s, critical heartbeat-liveness.
- Input: suspect 2 s, stalled 10 s, critical heartbeat-liveness.

Filesystem, Network, Audio, GPU, and Storage remain visible as Offline until their own authoritative producers are wired. This avoids false confidence from synthetic heartbeats.

## Qualification performed
- `NUCLEUS HEALTH P1 PASS`: deterministic Healthy -> Suspect -> Stalled -> Healthy recovery, including a live heartbeat with a frozen progress token.
- `NUCLEUS P1 STATIC PASS`.
- Application containment survivability regression: PASS.
- Pass 13 crash isolation regression: PASS.
- Solum/Draw/Forge integration: 16/16 PASS.
- Draw core/codecs/app and Forge runner contract: PASS.
- Production EFI artifact verification: PASS.

## Production artifact
BOOTX64.EFI SHA-256: `da89ea79f0e5b09441aaab6551b2d168f4fc5d9e6ef4d80a7503b694f70fb31e`

## Pass 1 acceptance rule
A subsystem may be classified Suspect or Stalled, and Nucleus may report/log that condition, but **Nucleus Pass 1 must take no recovery action**. Automatic recovery begins only in Pass 2 after detector behavior is hardware-qualified.
