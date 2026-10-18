# Amateur-radio mesh changes — R58

**Plan A / Plan B / Plan C through Task 2A.** Radio scope is native C++, principally receive-only.

- New `include/trinity/radio/*` and `src/radio/*`: AX.25, mesh protocol, link and crypto scaffolding, AFSK1200 Bell-202 modem with 1200 bit/s frames and audio conversion.
- New audio capture infrastructure: capture-only HDA stream/BDL/buffer, default-denied privacy and revoke-on-fault/reset, separate playback ownership.
- Radio capture syscall numbers **83, 84, 85**, bounded to 256 frames per read.
- USB discovery, claim tokens and endpoint isolation modules with protection for USB HID and mass storage. USB interface syscalls **86–90**; transfer calls **88/89** are intentionally denied until actual xHCI hardware transfer programming is implemented.
- Private Ring-3 `RadioLinkWorker` slot **28**, non-graphical, receive-only and denied RF transmit/PTT, generic USB, network fetch and filesystem write capabilities; Mesh graphic slot 26 and reserved worker slot 27.
- Build-time `RADIO_LINK.ELF` embedding and validation.
- Original radio overlay: **65 added files, 23 modified files, 0 deleted**; manual merges of `include/trinity/usb.hpp`, `src/kernel/usb.cpp`, `src/kernel/gui_runtime.cpp` preserved FAT32/USB GUI boundaries.

**Qualified:** host radio protocol, AFSK receive, isolation and endpoint resource tests; kernel/EFI compile. **Not qualified:** physical Baofeng receive/transmit, CH340/CH341 transfer, PTT, radio programming or actual RF mesh store-forward.

Review `RADIO_MESH_PLAN_B_FINAL_QUALIFICATION_REPORT.md`, `RADIO_MESH_PLAN_C_TASK2A_QUALIFICATION_REPORT.md`, and `TRINITY_R58_REALFS_MESH_MERGE_REPORT.md` in the full source.
