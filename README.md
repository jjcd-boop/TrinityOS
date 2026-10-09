# Trinity OS

Trinity OS is an independently developed x86-64 operating-system project.

## Current milestone

**Multicore Boot Auto-Admission R3 — hardware qualified on the Acer Nitro 5.**

> Multi-core fully enabled into the high-half kernel. Automatically enters desktop with all 8-cores active.

The current qualified boot path automatically admits CPU2 through CPU7 after desktop entry. The production runtime reaches `hw-online=8` and `sched-online=8` without requiring the manual `MC2START` through `MC7START` sequence. The original manual controls remain available for diagnostics and recovery.

See `SOURCE_SNAPSHOT.md` and `passes/multicore-auto-r3/` for the authoritative snapshot identity, hardware evidence, engineering report, and qualification notes.

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
