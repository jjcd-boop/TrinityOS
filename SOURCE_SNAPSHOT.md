# Authoritative Trinity OS Source Snapshot

Current licensed source package:

`TrinityOS_MULTICORE_BOOT_AUTO_ADMISSION_R3_SOURCE.zip`

SHA-256:

`e1c160ede4b406dcb2ea869eb5b554cd545165936fb1b5d2ec9c86898d22306c`

Current FAT32 USB test package:

`TrinityOS_MULTICORE_BOOT_AUTO_ADMISSION_R3_USB.zip`

SHA-256:

`11d15e4382d888e22596291bcc78d44794133ac59232ded58acf969ba0f2b875`

Current `BOOTX64.EFI` SHA-256:

`6b4b4ba39e2bdcbef3331a39a55018c222792628036cdc06c00a13fc443d80ea`

## Current pass

**Multicore Boot Auto-Admission R3 — HARDWARE QUALIFIED**

Release note:

> Multi-core fully enabled into the high-half kernel. Automatically enters desktop with all 8-cores active.

Hardware qualification on the Acer Nitro 5 confirmed automatic admission of CPU2 through CPU7 during normal desktop boot. The post-boot `MC` evidence reported:

`MC discovered=8 preparedAP=0 hw-online=8 sched-online=8 cap=8 runnable=0`

and:

`AUTO status=2 admitted=6 next=8 target=8 failcpu=none code=0`

CPU2 through CPU7 each retained hardware-online, scheduler-online, TLB, timer, and IRQ authority. No manual `MC2START` through `MC7START` commands were required.

The R3 boot coordinator drives the same hardware-qualified `GuiRuntime / MulticoreControl` transactions previously proven by the manual multicore qualification chain. APs are admitted sequentially and fail closed: a contained AP failure stops later automatic admissions for that boot while preserving the already-qualified runtime.

The repository contains the R3 engineering evidence and integration notes under `passes/multicore-auto-r3/`.

Copyright (c) 2026 James Davis. Third-party materials remain governed by their respective licenses.
