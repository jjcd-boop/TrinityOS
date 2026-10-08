# Nucleus Pass 1 Hardware Test

1. Unzip the EFI package to the FAT32 Trinity USB and boot normally.
2. Reach the desktop and leave the system idle for at least 30 seconds.
3. Open Console and run `NUCLEUS`.
4. Confirm the summary reports zero `critical-stalled` entries.
5. Confirm KERNEL, SCHEDULER, DESKTOP, COMPOSITOR, and INPUT are registered and HEALTHY after the desktop is active.
6. Leave Console open and exercise Draw, Forge, Writer, mouse movement, and window dragging, then run `NUCLEUS` again.
7. Confirm heartbeat/progress counters advanced and no subsystem remains SUSPECT/STALLED.
8. If a subsystem transitions, photograph the complete `NUCLEUS` output and note what was happening immediately beforehand.

Expected limitation for Pass 1: Filesystem, Network, Audio, GPU, and Storage may report OFFLINE because their authoritative heartbeat producers are intentionally deferred rather than faked.

Pass criteria: detector remains stable, does not create boot/desktop regressions, and does not perform automatic recovery or termination.
