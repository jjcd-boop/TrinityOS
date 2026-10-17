# Trinity OS

Trinity OS is an independently developed x86-64 operating-system project. It aims to become a microkernel, with native amateur radio and artificial intelligence support. 

## Current milestone

**Desktop Functionality R2 (2026-10-11)** on top of the Desktop + Browser Usability R1 / Browser P15 R5 line and the **VM Pass 3C-R2 Production Cleanup / Freeze** baseline.

This milestone expands practical desktop functionality without enlarging the existing taskbar/dock. Clicking the clock now opens a month calendar; the compact top status area opens Network, Volume, USB, and Quick Settings flyouts; `Alt+Tab` / `Super+Tab` switches among open applications; and `Super+R` opens a Run box for launching Trinity applications.

File Explorer now includes an editable address/path bar, New Folder, Rename, and Trash controls with confirmation. The new Rename/Delete syscall surface is intentionally bounded to Trinity's current compatibility VFS path and does not bypass native TFS/FSSVC durability semantics.

The previous usability pass remains present: Trinity Control is integrated into the fixed-height top taskbar immediately left of Nucleus, temporary notifications are supported, window snapping is available, ZIP extraction is bounded, local images have a viewer path, and common text/source/config formats open through Writer.

Browser usability remains on the P15 R5 line. On the Acer Nitro 5 hardware test system, Trinity's Realtek Ethernet path was verified operational and the Trinity browser successfully connected to **Google over Ethernet**. This is a verified hardware observation, not a declaration of complete web compatibility; Yahoo, YouTube, CNN Lite, AOL, and other complex sites remain active compatibility targets.

Intel AX200 Wi-Fi remains a separate integration item. The current source package does not contain a valid AX200 firmware blob, so the build reports AX200 firmware absent/disabled.

The underlying VM baseline combines the hardware-qualified firmware-derived multicore path with Trinity's current per-process virtual-memory milestone. On the Acer Nitro 5, Trinity has been hardware-qualified with all 8 logical processors active; the current software topology/descriptor capacity is 32 logical processors, with >8 still awaiting hardware qualification.

See `SOURCE_SNAPSHOT.md`, `passes/desktop-functionality-r2/`, `passes/desktop-browser-usability-r1/`, and `passes/vm-pass3c-r2/` for source deltas, hashes, audits, milestone history, current limitations, and restart guidance.

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
