# Trinity OS Source Snapshot

Current published source baseline: **VM Pass 3C-R2 Production Cleanup / Freeze (2026-10-10)**.

This snapshot incorporates the hardware-qualified dynamic multicore baseline and the current hardware-qualified virtual-memory milestone.

Key state:

- firmware-derived multicore target, hardware-qualified with 8/8 logical CPUs active on the Acer Nitro 5;
- current software CPU capacity: 32 logical processors (architectural support; >8 not yet hardware-qualified);
- per-process private CR3/VM ownership sidecars;
- VM-region metadata and mapping permission audits;
- private Ring-3 page-fault attribution;
- sparse anonymous first-touch zero-fill allocation;
- 256 KiB per-process sparse heap-style reservation for private graphical apps;
- bounded persistent anonymous-page reclaim;
- AP Ring-3 retirement-to-kernel-idle fallback for deterministic process teardown;
- artificial VM qualification startup probe removed for the production freeze.

See `passes/vm-pass3c-r2/VM_PASS3C_R2_PRODUCTION_FREEZE_REPORT.md` for the full milestone history, qualification notes, limitations, and recommended future VM roadmap.

Full source archive:

`TrinityOS_VM_PASS3C_R2_PRODUCTION_FREEZE_SOURCE.zip`

SHA-256: `b13fd6a8d39ea0e0c664ca56b01bf50b4cfe1ace9454dde9cdcaec0d475fb30c`

FAT32 EFI tree archive:

`TrinityOS_VM_PASS3C_R2_PRODUCTION_FREEZE_EFI.zip`

SHA-256: `afe322d3370adf43baef6023cd6c9beab60b4b772eb4b7e8624d3a98a6e842f3`

Production `BOOTX64.EFI` SHA-256 for this source snapshot:

`ee0d58b6d473f08e2512e3b84db87f0565b3d5c8ec0b8188886cebd0c515bcfa`
