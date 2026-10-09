# Trinity OS Multicore Boot Auto-Admission R3 — Release Notes

**Hardware status:** QUALIFIED on Acer Nitro 5

> Multi-core fully enabled into the high-half kernel. Automatically enters desktop with all 8-cores active.

R3 completes production integration of the previously hardware-qualified multicore admission path. Trinity now boots into the desktop with CPU0 through CPU7 active without requiring manual `MC2START` through `MC7START` commands.

Post-boot hardware evidence:

`MC discovered=8 preparedAP=0 hw-online=8 sched-online=8 cap=8 runnable=0`

`AUTO status=2 admitted=6 next=8 target=8 failcpu=none code=0`

The manual admission commands remain present for diagnostics and recovery. R28 stress and R29 production-soak controls remain explicit operator commands.
