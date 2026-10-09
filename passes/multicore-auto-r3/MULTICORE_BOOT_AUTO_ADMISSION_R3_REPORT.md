# TrinityOS Multicore Boot Auto-Admission R3

## Hardware result from R2

BOOT-AUTO R2 did not activate CPU2-CPU7 on the Acer Nitro 5. Two `MC` checks after normal desktop entry still showed only CPU0 and CPU1 online. R29-R1 therefore remained the authoritative hardware-qualified rollback baseline while R3 was prepared.

The R2 build did not expose its automatic-admission state through `MC`, so the hardware evidence could not distinguish between "the hidden Service-side trigger never crossed every gate" and "it triggered and the first controller transaction contained." R3 did not guess between those cases. It removed that hidden trigger design entirely and added operator-visible automatic-admission telemetry.

## R3 integration design

R3 automates the same `GuiRuntime / MulticoreControl` transactions used by the hardware-qualified manual commands.

The private Desktop now:

1. publishes its first normal frame;
2. enters the normal desktop service loop;
3. waits a bounded 500 ms using the platform TSC frequency;
4. queries the kernel's read-only boot-auto status to learn the discovered/eligible CPU target;
5. submits one explicit `MulticoreControl` transaction for CPU2;
6. after success, waits 100 ms and submits CPU3;
7. repeats through the last discovered CPU, up to the current eight-CPU architecture limit;
8. stops immediately on any contained failure.

This is materially closer to the proven operator path than R1/R2. There is no AP admission hidden inside `ScreenWidth`, `Service`, `PresentFrame`, or another unrelated syscall. Each processor receives its own explicit controller transaction and returns to Ring 3 before the next AP is requested.

The Desktop is hard-affined to CPU0 by the production scheduler policy, so the automatic request executes from the same CPU0 Desktop controller domain used by the successful manual qualification sequence.

## Kernel changes

`gui::BootAutoAdmissionGeneration` marks only Desktop-owned boot-auto requests; it does not change the existing `MulticoreControl` message layout.

`GuiRuntime/MulticoreControl payload=30` is a read-only boot-auto status query. It publishes:

- status: 0 pending, 1 running, 2 complete, 3 contained;
- admitted AP count;
- next CPU;
- discovered/eligible target count;
- failing CPU and failure code, if any;
- current online/scheduler masks.

Automatic CPU2-CPU7 requests still call `smp::stage_mc2_manual_hardware_admission()` unchanged. R28 and R29 are never executed automatically.

`MC` now prints an `AUTO ...` line so a failed boot can distinguish trigger failure from a specific contained admission checkpoint without serial capture.

## Safety / rollback

- CPU0 and CPU1 remain the starting authoritative pair.
- APs are admitted strictly in order.
- A failed AP stops later automatic requests for that boot.
- No automatic retry occurs after a contained failure.
- Manual `MC2START` through `MC7START` remain available for diagnostics.
- R28 stress and R29 soak remain explicit operator commands.

## Software qualification

- BOOT-AUTO R3 integration audit: 15/15 PASS
- R29-R1 launch-liveness regression: 8/8 PASS
- R29 production-soak regression: 31/31 PASS
- R28-R5 persistent AP identity regression: 13/13 PASS
- R28 all-CPU stress regression: 41/41 PASS
- R27 CPU7 admission regression: 69/69 PASS
- R23 generalized AP admission regression: 79/79 PASS
- Pass 15 multicore diagnostics: 22/22 PASS
- Pass 16 graphical qualification: 33/33 PASS
- changed translation units, warnings-as-errors: PASS
- private Desktop ELF: 3774 transactional pages
- full EFI link: PASS
- artifact verification: PASS

## Hardware qualification

R3 subsequently passed hardware qualification on the Acer Nitro 5. A fresh normal boot, without manual AP-start commands, reported:

`MC discovered=8 preparedAP=0 hw-online=8 sched-online=8 cap=8 runnable=0`

and:

`AUTO status=2 admitted=6 next=8 target=8 failcpu=none code=0`

This promotes R3 to the authoritative production multicore boot baseline for this machine.
