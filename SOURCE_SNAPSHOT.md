# Trinity OS Source Snapshot

Current working source baseline: **Desktop + Browser Usability R1 (2026-10-11)**, built on the hardware-qualified **VM Pass 3C-R2 Production Cleanup / Freeze** baseline and the Browser P15 R5 development line.

## Current usability state

- Trinity Control moved from the floating desktop HUD into the existing fixed-height top taskbar immediately left of Nucleus.
- The taskbar was not expanded and no additional desktop working area is consumed.
- Trinity Control now launches Settings / Control Panel directly.
- Browser usability has advanced through the P15 R5 line.
- Realtek wired Ethernet is operational in the current desktop path.
- The Trinity browser has been hardware-observed successfully connecting to **Google over Ethernet** on the Acer Nitro 5.
- Intel AX200 Wi-Fi startup remains deferred and is not included in the Google-over-Ethernet verification.

## Preserved architecture baseline

- firmware-derived multicore target, hardware-qualified with 8/8 logical CPUs active on the Acer Nitro 5;
- current software CPU capacity: 32 logical processors (architectural support; >8 not yet hardware-qualified);
- per-process private CR3/VM ownership sidecars;
- VM-region metadata and mapping permission audits;
- private Ring-3 page-fault attribution;
- sparse anonymous first-touch zero-fill allocation;
- 256 KiB per-process sparse heap-style reservation for private graphical apps;
- bounded persistent anonymous-page reclaim;
- AP Ring-3 retirement-to-kernel-idle fallback for deterministic process teardown.

## Published source delta and reports

See:

- `passes/desktop-browser-usability-r1/TRINITY_CONTROL_TASKBAR_R1.patch`
- `passes/desktop-browser-usability-r1/TRINITY_CONTROL_TASKBAR_R1_REPORT.md`
- `passes/desktop-browser-usability-r1/DESKTOP_BROWSER_USABILITY_R1_RELEASE_NOTES.md`
- `passes/vm-pass3c-r2/` for the underlying production source snapshot and VM baseline.

Working source archive:

`TrinityOS_TASKBAR_TRINITY_CONTROL_R1_SOURCE.zip`

SHA-256:

`33025a39988518398a3a40fb517132c9f98bcab099e86c45540deac23962afd7`

Updated `src/user/desktop.cpp` SHA-256:

`cf4f1a6fb04360c99943c5d766cea283de42a501fff673a827b0f45e73d941e7`

## Verification language

The Trinity Control taskbar integration is build-verified. Google-over-Ethernet is a hardware-observed browser/network result. This snapshot does not claim full browser compatibility and does not mark AX200 Wi-Fi as hardware-qualified.
