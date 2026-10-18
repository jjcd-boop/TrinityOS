# Real filesystem changes — R58

- Moved primary FAT32/FSSVC and optional physical exFAT sources from `src/staged/storage` to `src/storage`.
- Retired the TrinityFS/TFS1 production backend, TFS source/test paths and `TRINITY.TFS` packaging.
- Removed old kernel RAM-file, staged Desktop/Documents, session-save and UEFI-variable document stores.
- Normal filesystem list/read/write routes through physical FSSVC and fails closed; syscall **40** remains ABI-reserved and has no session-save handler.
- File Explorer uses canonical FAT32 user directories, with legacy aliases resolving to the physical namespace. App document-open handoff passes a pathname, not a staged file.
- Audited Writer, Sheets, Calculator, Forge and Desktop state save paths switched to scoped asynchronous FSSVC operations.
- Added **32 real files** as Browser/media/Solum resource staging data for unzip-to-FAT32 USB installation.
- Physical FAT32 and explicitly mounted exFAT retained; the old USB vault-era helper/diagnostic code still needs final cleanup.

**Hardware qualification has failed for async saves:** Writer `NOT COMMITTED`; Writer/Sheets/Slides `ASYNC SAVE QUEUE FAILED`. FSSVC queue admission/completion, path permissions and retirement cleanup require repair. Explorer can see an image placed on FAT32 from Windows; opening it in Browser may fail due to a local-read size-limit mismatch. The standalone disk `.img` does not embed the supplemental physical resources.

**Software status:** merged kernel/EFI build and targeted namespace/broker/static tests passed. **Not yet hardware qualified:** durable Windows ↔ Trinity write/readback and general app save/reopen.
