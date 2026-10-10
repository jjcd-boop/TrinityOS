# Trinity OS

Trinity OS is an independently developed x86-64 operating-system project.

## Current milestone

**VM Pass 3C-R2 — Production Cleanup / Freeze (2026-10-10).**

This baseline combines the hardware-qualified firmware-derived multicore path with Trinity's current per-process virtual-memory milestone. On the Acer Nitro 5, Trinity has been hardware-qualified with all 8 logical processors active; the current software topology/descriptor capacity is 32 logical processors, with >8 still awaiting hardware qualification.

The VM baseline now includes private per-process CR3/address-space ownership, explicit VM regions, pre-admission mapping and guard audits, private Ring-3 page-fault attribution, sparse anonymous first-touch zero-fill allocation, a 256 KiB per-process sparse heap-style reservation, bounded persistent anonymous-page reclamation, and the generic AP Ring-3 retirement-to-kernel-idle scheduler repair discovered during teardown qualification.

Pass 3C-R2 removes the temporary startup heap-touch qualification probe so anonymous memory is now committed only by real application access. It intentionally freezes the VM subsystem before growable heaps, mapped files, copy-on-write, page replacement, or swap.

See `SOURCE_SNAPSHOT.md` and `passes/vm-pass3c-r2/` for the source slice, production hashes, audits, milestone history, current limitations, and restart plan.

## License

Trinity OS is **source-available for noncommercial use** under the `Trinity OS Noncommercial Source License 1.0`. It is not distributed under an OSI-approved open-source license.

In general, the license permits personal, educational, research, evaluation, hobbyist, and other noncommercial use; source inspection; noncommercial modification; and qualifying noncommercial redistribution. Commercial use, paid redistribution, monetization, and relicensing require separate written authorization from the Copyright Owner.

See:
- `LICENSE`
- `COPYRIGHT`
- `NOTICE`
- `TRADEMARKS`
- `CONTRIBUTING.md`
- `CONTRIBUTOR_COPYRIGHT_ASSIGNMENT_AGREEMENT.md`
- `THIRD_PARTY_NOTICES.md`

## Contributions

Contributions are welcome. To preserve centralized ownership and a clear chain of title, an outside contribution may be accepted into the official Trinity tree only after the contributor signs the Trinity Contributor Copyright Assignment Agreement or another written agreement approved by the Copyright Owner.

## Third-party material

Third-party components and reference-derived material remain governed by their respective licenses. The Trinity license does not supersede third-party rights or obligations.

Copyright (c) 2026 James Davis.
