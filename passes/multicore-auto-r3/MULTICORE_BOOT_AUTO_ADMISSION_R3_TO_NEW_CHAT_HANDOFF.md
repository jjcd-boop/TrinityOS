# TrinityOS Multicore Boot Auto-Admission R3 — New Chat Handoff

## Authoritative hardware baseline

BOOT-AUTO R3 is hardware-qualified on the Acer Nitro 5. A normal boot now automatically admits CPU2 through CPU7 and reaches all eight logical processors online without manual `MCxSTART` commands.

Hardware evidence:

`MC discovered=8 preparedAP=0 hw-online=8 sched-online=8`

`AUTO status=2 admitted=6 next=8 target=8 failcpu=none code=0`

R28-R5 previously completed the 64-round eight-CPU stress test with `65535/65535`; R29-R1 completed the normal-production soak with `65535/65535` using the same underlying multicore admission/runtime machinery.

## Failed integration attempts

BOOT-AUTO R1 and R2 both booted to the desktop but `MC` still showed only CPU0 and CPU1 online. They are superseded by R3.

## R3 design

R3 removes hidden admission from unrelated syscall side effects. The hard-CPU0-owned Desktop waits 500 ms after entering its normal post-first-frame loop and then issues the existing `GuiRuntime/MulticoreControl` admission transaction once per AP, CPU2 through the discovered target. A 100 ms gap separates successful AP transactions. The kernel admission engine itself is unchanged.

`MC` prints:

`AUTO status=<0..3> admitted=<n> next=<cpu> target=<count> failcpu=<cpu|none> code=<failure>`

Interpretation:
- 0 = coordinator never started;
- 1 = automatic sequence is currently running;
- 2 = automatic sequence completed;
- 3 = contained failure; use failcpu/code to diagnose.

## Next work

Treat R3 as the production multicore boot baseline. Further multicore work should focus on portability to additional processor/topology families, scheduler policy/performance, and regression protection rather than repeating Acer Nitro 5 CPU bring-up.
