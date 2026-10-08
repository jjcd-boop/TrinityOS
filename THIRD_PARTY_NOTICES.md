# Trinity OS Third-Party Notices

Third-party material is excluded from the ownership claim and license grant covering original Trinity material. Each third-party component remains governed by its own license. The notices below are preserved as part of the Trinity distribution.

# Third-Party Notices — GPU P15-R1

## Intel GPU Tools (IGT) — Gen9 render-copy reference

Trinity's P15 Gen9.5 3D bring-up uses the public Intel GPU Tools Gen9 render-copy implementation as a programming reference for command/state ordering.
The 64-byte Gen9 pixel-shader program in `src/kernel/gpu.cpp` is adapted from:

- Project: IGT GPU Tools
- Version/reference: upstream v1.28
- File: `lib/rendercopy_gen9.c` (`ps_kernel_gen9`)
- Upstream project: https://gitlab.freedesktop.org/drm/igt-gpu-tools
- Mirrored reference used during this pass: https://chromium.googlesource.com/chromiumos/third_party/igt-gpu-tools/+/refs/tags/upstream/v1.28/lib/rendercopy_gen9.c
- License: MIT/X11 family (upstream `COPYING`)

Relevant Intel notice carried by IGT's `COPYING`:

Copyright © 2006-2011 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next paragraph) shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

This notice covers the adapted shader bytecode and the IGT-derived Gen9 state sequencing concepts used by the P15 implementation. Trinity-specific ownership, GGTT rollback, RCS takeover/recovery, graphics-service integration, diagnostics, and hardware-qualification policy are separate Trinity code.

## NVIDIA TU117 firmware (not bundled)

The P20/P21 test package may include `FETCH_NVIDIA_TU117_FIRMWARE.ps1`, which retrieves unmodified firmware directly from NVIDIA's `linux-firmware` repository at a pinned revision. The firmware binaries are not included in Trinity source or USB archives produced by this pass. Their use is governed by NVIDIA's `LICENCE.nvidia` terms in that repository.

## Linux Nouveau — Turing ACR/Falcon programming reference

Trinity's P21 TU117 authenticated-ACR boundary uses the public Linux Nouveau implementation as a programming reference for Turing ACR sequencing, Falcon high-security firmware descriptors, Falcon IMEM/DMEM PIO loading, instance/VMM binding, and WPR hardware verification. Trinity's implementation is written for its own supervisor mapping, DMA admission, diagnostics, rollback policy, and Intel-owned recovery/display model; it does not embed Linux/Nouveau source.

Reference files reviewed from the Linux kernel tree include:

- `drivers/gpu/drm/nouveau/nvkm/subdev/acr/tu102.c`
- `drivers/gpu/drm/nouveau/nvkm/subdev/acr/gp102.c`
- `drivers/gpu/drm/nouveau/nvkm/subdev/acr/gp108.c`
- `drivers/gpu/drm/nouveau/nvkm/subdev/acr/gm200.c`
- `drivers/gpu/drm/nouveau/nvkm/falcon/fw.c`
- `drivers/gpu/drm/nouveau/nvkm/falcon/gm200.c`
- `drivers/gpu/drm/nouveau/nvkm/falcon/gp102.c`
- `drivers/gpu/drm/nouveau/include/nvfw/acr.h`
- `drivers/gpu/drm/nouveau/include/nvfw/flcn.h`

These Nouveau files carry an MIT/X11-family permission notice, including work copyrighted by Red Hat and their respective authors. The relevant permission terms are:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to inclusion of the copyright and permission notice in substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
