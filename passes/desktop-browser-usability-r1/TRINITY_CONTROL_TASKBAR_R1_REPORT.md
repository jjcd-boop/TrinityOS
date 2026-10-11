# Trinity Control Taskbar Integration R1

## Scope
Move the floating `TRINITY CONTROL` desktop HUD into the existing top taskbar without increasing the taskbar height or consuming additional desktop workspace.

## Changes
- Removed the 296x118 floating `TRINITY CONTROL` HUD from the upper-right desktop canvas.
- Added a compact 142x32 `TRINITY CONTROL` control to the existing 46px top-bar envelope.
- Positioned Trinity Control immediately to the left of the existing `NUCLEUS` control with a 10px gap.
- Preserved the existing Nucleus location and top-bar height.
- Made the Trinity Control top-bar control actionable: clicking it opens the Settings/Control Panel process (`process 4`).
- Removed the no-longer-used floating HUD status-chip helper.
- Left the normal Settings application and its functionality unchanged.

## Build verification
`scripts/build_desktop_first_elf.sh` completed successfully.

Verified desktop ELF:
- Size: 4,118,520 bytes
- Entry: `0x40000000`
- PT_LOAD segments: 3
- Transactional pages: 3445
- Browser stack gate: PASS
- SHA-256: `b155ffdd6cb68a5efb0f595f1b1fd949e5f8f3d119dfdddebcdbf9ce002e07dd`

## Packaging update
After the source-only integration report was produced, the full EFI build was also completed successfully and an unzip-to-FAT32 USB package was generated as `TrinityOS_TASKBAR_TRINITY_CONTROL_R1_USB.zip`.
