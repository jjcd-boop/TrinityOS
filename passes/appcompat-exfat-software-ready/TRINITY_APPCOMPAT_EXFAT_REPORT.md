# Trinity AppCompat exFAT Software Compatibility Report

**Date:** 2026-10-16
**Phase 9E software-ready parent:** `af5a5b4`
**Approved AppCompat design:** `e3193bd`
**Approved AppCompat implementation plan:** `e666994`
**Qualified AppCompat code checkpoint:** `faec0db`
**State:** **SOFTWARE-QUALIFIED — PHYSICAL EXFAT APPLICATION WRITES REMAIN DISABLED PENDING PHASE 9E HARDWARE PASS**

## Purpose

Prepare every current Trinity application/component that persists user-created or user-selected data to operate through one backend-neutral VFS/FSSVC path when a writable exFAT volume is eventually physically admitted. The qualification in this report uses simultaneous writable TrinityFS root storage plus **RAM-backed simulated exFAT** under `/Volumes/TESTEXFAT`.

This pass does **not** bypass Phase 9E. It does not arm the physical writer, grant block/device authority to applications, or make Phase 9E hardware-qualified.

## Save-capable inventory

Machine-checked qualified set:

- Writer — document save/reopen/overwrite and external Save As.
- Sheets — XLSX/CSV save/reopen/overwrite and external Save As.
- Slides — `.tsl` save/reopen/overwrite and external Save As using asynchronous durable persistence.
- Draw — `.tdraw` Save As with full external path support.
- Forge — `.forge` project persistence with retained absolute project path.
- Browser — file-backed profile/download/page persistence remains backend-neutral; fixed logical profile/download roots stay on the OS root unless an explicit export path is supplied by a future UI flow.
- Calculator — fixed-root user export remains on `/Documents`.
- File Explorer — external-volume directory creation, copy/extract write, enumerate, same-mount rename, external Trash/remove, and deterministic cross-mount rename rejection.

Explicit classifications:

- Settings/Desktop preferences — fixed-root system-state persistence, not redirected to exFAT.
- Notes — RAM-only in the current source; durable Notes save is a separate feature.
- Media/Photo — read-only in the current source unless a future export/write path is added.

The persistence inventory test fails if a newly discovered durable write path is not explicitly classified.

## Architecture delivered

- TFS remains mounted at `/`.
- A bounded, allocation-free FSSVC-local router reuses the existing VFS mount namespace and trusted mount-target registry.
- Simulated exFAT is explicitly attached under `/Volumes/<label>/`.
- Existing FSSVC protocol/syscall ABI is preserved.
- Applications continue using normal VFS/FSSVC operations; no app branches on TFS vs exFAT.
- Routed handle identity includes a mount/target discriminator internally while preserving the public `ObjectHandle` ABI.
- Cross-mount rename returns `Result::Unsupported`; no false atomicity is reported.
- `/Volumes` is a reserved virtual namespace. Unknown, detached, revoked, stale, unmounted, recovery-required, or epoch-mismatched external targets fail closed and do not fall through to TFS.
- Production FSSVC/full-build source lists include `vfs_mount_namespace.cpp`, `vfs_mount_targets.cpp`, and `fs_service_router.cpp`.

## Checkpoints

- **AppCompat-A — router + persistence inventory:** `3f54d70`
- **AppCompat-B — routed FSSVC + mount-safe handles:** `2acc8e3`
- **AppCompat-C — backend-neutral application Save As paths/status:** `55adda9`
- **AppCompat-D — all-save-capable routed persistence matrix:** `dd1e898`
- **AppCompat-E — Explorer external-volume mutation qualification:** `fb24085`
- **AppCompat-F review fix — Slides async durable routed save:** `9bb4a01`
- **AppCompat-F qualification harness/build inclusion:** `0147a9f`
- **AppCompat-F whole-branch review fix — detached external paths fail closed:** `faec0db`

## Qualification results

1. **Gate 1 — persistence inventory + path/status audit + physical-write/scope fence:** PASS.
2. **Gate 2 — bounded router + inherited mount namespace/target behavior:** PASS.
3. **Gate 3 — routed FSSVC + mount-safe handle identity:** PASS.
4. **Gate 4 — all-save-capable application persistence matrix:** PASS.
5. **Gate 5 — Explorer external-volume mutation matrix:** PASS.
6. **Gate 6 — ASan/UBSan routed/app/Explorer/handle paths:** PASS.
7. **Gate 7 — inherited TFS/FSSVC/exFAT + Phase 9D + Phase 9E software-safety regressions:** PASS in bounded parts.
8. **Gate 8 — production build inclusion, freestanding compile, real FSSVC ELF, hardware fence and diff hygiene:** PASS.

### Deterministic campaign

**100/100 PASS after the final whole-branch review fix.**

Each iteration exercised:

- routed dual-backend TFS + simulated exFAT persistence;
- the save-capable application persistence matrix;
- Explorer external-volume mutations and cross-mount denial;
- byte-pinned Phase-9E hardware boundary hashes.

No physical media was written. All exFAT mutation counts came from RAM-backed host-test media.

## Application persistence evidence

The matrix proves representative create/save/reopen/overwrite behavior for Writer, Sheets XLSX, Sheets CSV, Slides, Draw, Forge, and explicit exported page data on simulated exFAT. It separately proves Browser profile and Calculator export remain on the TFS root logical paths.

Fault coverage includes:

- definite exFAT write failure is never reported as successful;
- commit-critical flush ambiguity propagates as `Result::DurabilityUnknown`;
- ambiguous writes are not automatically replayed;
- revoked/stale external targets fail closed;
- detached or unknown `/Volumes/<label>/...` paths return `NotFound` rather than falling through to TFS.

## Explorer evidence

PASS for:

- virtual `/Volumes` enumeration;
- `/Volumes/TESTEXFAT` mount projection;
- external directory creation;
- copy/extract-style file write;
- external enumerate/readback;
- same-mount rename;
- external mount-local Trash move and permanent delete;
- TFS -> exFAT and exFAT -> TFS rename rejection with source preservation.

Cross-mount rename remains intentionally `Unsupported`; copy+verify+delete is not falsely advertised as atomic rename.

## Whole-branch review

A fresh reviewer subagent is not available in this harness, so the required whole-branch review was performed as a separate author self-review and recorded in the execution ledger. This is weaker than an independent reviewer and should be treated accordingly before a shared-branch merge.

The review found two Important integration/safety defects, both fixed RED->GREEN before seal:

1. Slides still used legacy synchronous `WriteFileSession` and lacked the async durable file capability. It now uses `WriteFileAsync`/`PollFileWrite`, is restricted to `.tsl`, and may write only allowed local document roots or safe external-volume children.
2. Detached/unknown `/Volumes/<label>/...` paths could resolve to the root catch-all. `/Volumes` is now reserved: child paths require an active external mount and otherwise fail `NotFound`.

Review focus after fixes:

- same object IDs on separate mounts remain independent: PASS;
- stale/revoked/unmounted/recovery/epoch-drift targets fail closed: PASS;
- mount-boundary path escape is rejected: PASS;
- cross-mount rename does not claim atomicity: PASS;
- `DurabilityUnknown` reaches the caller without success/replay: PASS;
- persistence inventory drift fails the seal: PASS;
- no root fallback for detached/unknown external labels: PASS.

## Phase 9E physical-write fence

The following Phase-9E physical boundary files are byte-pinned to the software-ready `af5a5b4` checkpoint by `tests/appcompat_physical_write_fence_static.py`:

- `include/trinity/usb.hpp`
- `include/trinity/storage/physical_write_authority.hpp`
- `include/trinity/storage/storage_broker.hpp`
- `src/kernel/phase9e_usb_guard.cpp`
- `src/kernel/phase9e_hardware_qualification.cpp`
- `src/kernel/usb.cpp`
- `src/kernel/storage_broker.cpp`
- `src/nucleus/nucleus.cpp`

The audit also verifies that application, FSSVC, syscall, and router surfaces do not reference physical-write authority/USB BOT activation APIs and that AX200/Wi-Fi, Baofeng/amateur-radio, FunctionGemma/AI, audio, GPU, and SMP/scheduler workstreams were not modified by AppCompat.

**PHYSICAL EXFAT APPLICATION WRITES: DISABLED PENDING PHASE 9E HARDWARE PASS.**

## Build evidence

- New router/mount units compile freestanding.
- Modified Slides/syscall/GUI runtime units compile under the project single-unit build path.
- Real FSSVC ELF: PASS, **3 PT_LOAD segments, 880,448 bytes**.
- Private FSSVC and full Trinity build source lists include namespace/target/router units.
- Inherited Phase-8F physical read-only FSSVC host + ASan/UBSan tests: PASS after linking the now-required router units.
- Inherited Phase-9D controlled-transport gates 2-5: PASS.
- Inherited Phase-9E software-safety gates 1-6: PASS.

## Known unrelated baseline finding

The earlier AppCompat-C verification observed two unchanged ForgeRunner isolation-static failures in `scripts/verify_forge_r6.sh` that predate AppCompat and concern ForgeRunner isolation policy, not Forge project persistence. The modified Forge application itself compiled successfully as a private ELF. This AppCompat pass did not modify those unrelated isolation files.

## Current boundary

AppCompat is software-ready to travel beside the Phase-9E source. It must **not** be interpreted as permission to enable physical exFAT application writes.

Phase 9E still requires the real sacrificial `TRINITY9E` hardware PASS and final hardware-qualified seal. Phase 9F remains blocked until that occurs. Once the physical write/recovery sequence is qualified, the AppCompat router/application work can be connected to the admitted physical exFAT backend without redesigning the applications.
