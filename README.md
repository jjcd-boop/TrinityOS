# Trinity OS

Trinity OS is an independently developed x86-64 operating-system project.

## Current milestone

**Desktop + Browser Usability R1 (2026-10-11)** on top of the **VM Pass 3C-R2 Production Cleanup / Freeze** baseline.

The current usability milestone improves the day-to-day desktop and browser experience while preserving the established multicore and virtual-memory architecture. Trinity Control has been removed from the desktop workspace and integrated into the existing fixed-height top taskbar immediately to the left of Nucleus; the taskbar height was not increased. Trinity Control now launches the Settings / Control Panel surface directly.

Browser usability has also advanced through the P15 R5 line. On the Acer Nitro 5 hardware test system, Trinity's Realtek Ethernet path was verified operational and the Trinity browser successfully connected to **Google over Ethernet**. This is a verified hardware observation, not a declaration of complete web compatibility; Yahoo, YouTube, CNN Lite, AOL, and other complex sites remain active compatibility targets.

Intel AX200 Wi-Fi remains a separate integration item: wired Ethernet is functioning, while AX200 startup is still deferred in the production desktop-first networking path and requires its own boot-admission/MMIO/firmware integration pass.

The underlying VM baseline combines the hardware-qualified firmware-derived multicore path with Trinity's current per-process virtual-memory milestone. On the Acer Nitro 5, Trinity has been hardware-qualified with all 8 logical processors active; the current software topology/descriptor capacity is 32 logical processors, with >8 still awaiting hardware qualification.

The VM baseline includes private per-process CR3/address-space ownership, explicit VM regions, pre-admission mapping and guard audits, private Ring-3 page-fault attribution, sparse anonymous first-touch zero-fill allocation, a 256 KiB per-process sparse heap-style reservation, bounded persistent anonymous-page reclamation, and the generic AP Ring-3 retirement-to-kernel-idle scheduler repair discovered during teardown qualification.

See `SOURCE_SNAPSHOT.md`, `passes/desktop-browser-usability-r1/`, and `passes/vm-pass3c-r2/` for source deltas, hashes, audits, milestone history, current limitations, and restart guidance.

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
