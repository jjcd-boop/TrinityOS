# TrinityOS Desktop Functionality R2

Date: 2026-10-11

This pass adds a compact set of user-facing desktop functions without increasing the existing taskbar/dock height.

## Added
- Clock click opens a read-only month calendar with previous/next month navigation and today highlighting.
- Network flyout from the compact top-bar status region.
- Volume flyout with live volume slider and mute toggle.
- USB flyout showing xHCI/connected-port status and a shortcut into full Settings.
- Quick Settings flyout for theme, pointer sensitivity, and full Settings.
- Alt+Tab / Super+Tab application switching across open desktop windows.
- Super+R Run box for launching Trinity applications by name.
- File Explorer editable address/path bar (Ctrl+L or click).
- File Explorer New Folder action.
- File Explorer Rename action for currently mutable VFS entries.
- File Explorer Trash workflow with second-click confirmation; Trash view supports permanent deletion.

## Filesystem safety boundary
Rename/Delete syscalls were added to the current VFS mutation surface and capability classification. They do not bypass the native TFS/FSSVC durability model. When a selected object is owned by a storage path not currently mutable through this bounded VFS route, Explorer reports that the operation is unavailable instead of pretending it succeeded.

## Verification
- Browser stack gate: PASS.
- Desktop ELF verification: PASS, 3 PT_LOAD, transactional_pages=3572.
- FSSVC ELF verification: PASS.
- Private app fallback verification: PASS for Draw and Forge artifacts in this source package.
- Full EFI/USB build artifact verification: PASS.
- Browser P15 architecture checks: 23/23 PASS.
- BOOTX64.EFI SHA-256: `6eab8c45ce5d8d229a70334dcffd0646d6d73f0f6396a7dbb1ec40e90d93e96e`.
- Unzip-to-FAT32 USB ZIP SHA-256: `3bd412db4f7c3b829f0e152ed92f56d38a5e63b51a802de5a26fb1fbcf1ed2bd`.
- Full source ZIP SHA-256: `40a64b3b11a49ee01a8a04c50dc0928311563ac35543e6428a5174c0116071d7`.

## Known source-package limitation
The source package does not contain a valid AX200 firmware blob, so the build reports AX200 firmware absent/disabled. This pass does not change that Wi-Fi admission status.
