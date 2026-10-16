# Trinity AppCompat exFAT — Next Chat Handoff

**Date:** 2026-10-16
**Qualified code checkpoint:** `faec0db`
**Parent safety checkpoint:** Phase 9E software-ready `af5a5b4`
**State:** AppCompat software-qualified; Phase 9E hardware qualification still pending.

## What this branch adds

- simultaneous TFS root + simulated exFAT external mount routing through one FSSVC endpoint;
- trusted `/Volumes/<label>` namespace routing with no app filesystem-type knowledge;
- mount-safe handle identity;
- backend-neutral Save As handling for Writer, Sheets, Slides, Draw and Forge;
- full persistence classification for Browser, Calculator, Explorer, Settings/Desktop, Notes and Media/Photo;
- File Explorer external-volume mutation behavior;
- deterministic cross-mount rename rejection;
- fail-closed unknown/detached/revoked/stale external mount behavior;
- production FSSVC build inclusion for router/namespace/target units;
- 8-gate qualification and 100/100 deterministic campaign.

## Safety boundary

**PHYSICAL EXFAT APPLICATION WRITES ARE STILL DISABLED.**

AppCompat changed none of the byte-pinned Phase-9E hardware-writer/guard/broker/Nucleus files. No application, syscall, FSSVC client protocol, or router code receives raw block/USB/physical-write authority.

Do not turn the simulated external backend into a real writable USB backend until Phase 9E has a real sacrificial-media PASS and a final hardware-qualified seal.

## Current save-capable inventory

Qualified save-capable: Writer, Sheets, Slides, Draw, Forge, Browser, Calculator, File Explorer.

Classified separately:

- Settings/Desktop: fixed-root system-state persistence;
- Notes: RAM-only, no durable save implementation;
- Media/Photo: read-only in current source.

The inventory test must be rerun after future application additions. New persistence paths must be classified before the AppCompat seal is considered current.

## Qualification evidence

- AppCompat gates 1-8: PASS.
- Post-review deterministic routed compatibility campaign: **100/100 PASS**.
- Routed save/overwrite/reopen and failure/ambiguity behavior: PASS.
- Explorer external mutation/cross-mount safety: PASS.
- ASan/UBSan routed/application/Explorer/handle tests: PASS.
- Inherited TFS/FSSVC/exFAT, Phase-9D, Phase-9E software-safety regressions: PASS.
- Real FSSVC ELF: **880,448 bytes**, 3 PT_LOAD segments, verification PASS.
- Physical writes performed by this environment: **0**.

## Whole-branch review findings fixed

1. Slides legacy synchronous save path -> migrated to async durable routed save.
2. Detached/unknown `/Volumes/<label>` root fallback -> reserved external namespace now fails closed with `NotFound`.

Review was author self-review because this harness has no reviewer subagent. Before merging to a shared/upstream branch, an independent code review is still recommended.

## Merge guidance

The AppCompat branch is based directly on the Phase-9E software-ready chain. Preserve all AppCompat commits after `af5a5b4`; do not replace shared files wholesale when merging with contemporaneous Wi-Fi/Baofeng/AI work. Pay particular attention to:

- `src/kernel/syscall.cpp`
- `src/kernel/gui_runtime.cpp`
- `scripts/build.sh`
- `scripts/build_fs_service_elf.sh`

No AX200/Wi-Fi, Baofeng/amateur-radio, FunctionGemma/AI, audio, GPU, or SMP/scheduler files were intentionally changed by AppCompat.

## Next filesystem sequence

1. Run the existing Phase-9E sacrificial `TRINITY9E` hardware test.
2. If PASS, create final Phase-9E hardware-qualified seal.
3. Execute the already-prepared Phase-9F physical recovery workspace plan.
4. After physical recovery qualification, connect the admitted writable exFAT backend to this AppCompat router.
5. Phase 9G can then enable normal physical file create/replace through the same application-facing APIs already qualified here.
