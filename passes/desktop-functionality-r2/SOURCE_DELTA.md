# Desktop Functionality R2 — source delta

This milestone is built from the Desktop + Browser Usability R1 / Browser P15 R5 working tree.

Changed source files:

- `src/user/desktop.cpp` — calendar flyout, network/volume/USB/quick-settings flyouts, Run box, Alt+Tab/Super+Tab switching, compact top-bar status interaction.
- `src/user/apps/explorer.cpp` — editable address bar, New Folder, Rename, Trash confirmation/permanent-delete workflow.
- `include/trinity/syscall.hpp` — `RenamePath` and `DeletePath` syscall ABI plus request structures; dispatch table extended through syscall 75.
- `include/trinity/vfs.hpp` — bounded rename/delete VFS API declarations.
- `src/kernel/syscall.cpp` — file-mutation capability classification, user-pointer/path validation, handlers, dispatch registration.
- `src/kernel/gui_runtime.cpp` — GUI-domain capability admission for bounded RenamePath/DeletePath operations.
- `src/kernel/vfs.cpp` — Trash namespace plus bounded create/rename/delete support for the current compatibility VFS.

Safety boundary: native TFS/FSSVC-owned objects are not synchronously mutated through the compatibility route. Explorer reports an unavailable operation rather than bypassing FSSVC durability semantics.

Build verification: full EFI/USB artifact verification PASS. Desktop ELF transactional pages: 3572. Browser P15 architecture: 23/23 PASS. BOOTX64.EFI SHA-256: `6eab8c45ce5d8d229a70334dcffd0646d6d73f0f6396a7dbb1ec40e90d93e96e`.

Full working source archive produced for this milestone: `TrinityOS_DESKTOP_FUNCTIONALITY_R2_SOURCE.zip`, SHA-256 `40a64b3b11a49ee01a8a04c50dc0928311563ac35543e6428a5174c0116071d7`.
