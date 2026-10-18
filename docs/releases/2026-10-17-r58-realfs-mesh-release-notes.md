# TrinityOS R58 — RealFS + Baofeng Mesh integration (October 17, 2026)

**Integration candidate — source build and host-software tests passed; not hardware-qualified.**

This note describes the combined RealFS recovery source and the amateur-radio mesh Plan A/B/C-through-Task-2A overlay. Read `AMATEUR_RADIO_MESH_CHANGELOG.md` and `REAL_FILESYSTEM_CHANGELOG.md` for subsystem details.

## Amateur radio

Added AX.25 packet frames and mesh protocol/link/crypto code, Bell-202 AFSK1200 receive-side modem and audio conversion, dedicated privacy-controlled HDA capture, isolated Ring-3 RadioLink worker, USB interface discovery/claims, private endpoint resource/ring policy, capture syscalls 83–85 and USB interface syscalls 86–90. The merge brought **65 new files and 23 modified files**, with three manually resolved conflicts protecting RealFS and GUI permissions.

**Radio limits:** no hardware-proven Baofeng RX, no CH340/CH341 serial transfers or radio programming, no actual PTT/TX, no generic xHCI transfer completion, and no full over-the-air mesh. USB transfer syscalls 88/89 remain fail-closed.

## Filesystem

FAT32 is the primary physical writable filesystem via FSSVC with explicitly mounted exFAT retained; the old TrinityFS/TFS1 live backend, RAM/NVRAM/session VFS document store, session-write syscall 40 and `TRINITY.TFS` image generation were removed. Audited Writer, Sheets, Calculator, Desktop state and Forge save routes use physical FSSVC, and 32 Browser/media/Solum resources are included in the unzip-to-USB directory tree.

**Known persistence blockers:** Writer `NOT COMMITTED`; Writer/Sheets/Slides `ASYNC SAVE QUEUE FAILED`. Images are routed to Browser, where a local-read size mismatch can prevent opening. The generated 128 MiB `.img` does not include the supplemental USB resources. Some obsolete USB vault-era helpers remain. A successful build does **not** prove durable on-device persistence.

## Build and software verification

The merged R58 `bash scripts/build.sh` and FSSVC/private-worker builds passed. Radio Plan B/Plan C Task 2A host and isolation tests, seven radio static checks, FAT32 namespace/broker tests and Writer async static regression passed. Hardware RF and Windows↔Trinity durable-write qualification are outstanding.

See `TRINITY_R58_REALFS_MESH_MERGE_REPORT.md` in the complete source snapshot. Preserve the previous working R58 source/USB for rollback.

**License:** Trinity OS Noncommercial Source License 1.0; retains original `LICENSE`, `NOTICE` and third-party notices.
