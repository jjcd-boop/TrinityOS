# Trinity OS — experimental Rust PNG scanline worker (R1)

This is an **isolated, non-production prototype**, not a replacement for the R95 M1 source or the existing EFI. It does not change main, kernel code, storage, Wi-Fi, radio, or browser rendering.

## Scope
- Rust `#![no_std]` core with an `extern "C"` entry point
- Zero allocations, no OS dependencies, bounded input/output/scratch
- PNG filters None, Sub, Up, Average and Paeth, 8-bit RGB/RGBA only
- nearest-neighbor scale from a large inflated image to a bounded RGBA8 surface
- simple tests of reconstruction, bounds, and overlap failures
- C-compatible header for a **future** isolated Trinity Ring-3 decoder service

## Not yet implemented
- PNG file parser, chunk CRC checking, DEFLATE decompression, Adam7
- grayscale/palette/16-bit/ICC support, gamma/alpha color management
- Browser IPC/service admission, DMA, GPU, cache allocation or production integration
- freestanding Trinity-target compiler qualification or hardware verification

The caller must validate PNG and bound decompression **before** passing raw
scanlines. This separation allows the existing C++ DEFLATE implementation to be
assessed for reuse without introducing any third-party browser source.

## Host test commands
```sh
cargo test --manifest-path experiments/trinity_png_rows/Cargo.toml --lib
cargo build --release --manifest-path experiments/trinity_png_rows/Cargo.toml
```

The `staticlib` output is a candidate for a C++ ABI smoke test and later cross-target qualification, not a Trinity-ready executable.

## ABI notes
- `usize` in Rust maps to `size_t` in C on the same target.
- Valid, disjoint, correctly-sized buffers are mandatory for raw-pointer FFI.
- A panic traps in non-test x86-64 builds; production integration must use Trinity's exception/fault supervisor policy.
- Buffer ownership stays with the caller and Rust never retains pointers.
- Keep the code in a dedicated Ring-3 process after integration; don't link image parsing into Ring-0.

## Transition criteria
1. Compile on host with tests and sanitizers where supported.
2. Cross-compile for Trinity's real freestanding ABI.
3. C++/Rust ABI link and call on the supported execution target.
4. Fuzz malformed scanlines and prove upper memory bounds.
5. Introduce an IPC request/completion protocol and isolated process ownership.
6. Compare pixels against known-good decodes and measure cost versus the C++ path.

No code from Ladybird, WebKit or Servo was imported.
