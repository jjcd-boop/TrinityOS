# Trinity Browser Compatibility P1 — HTTP Session Foundation

Baseline: `TrinityOS_VM_PASS3C_R2_PRODUCTION_FREEZE_SOURCE.zip`

Baseline SHA-256: `b13fd6a8d39ea0e0c664ca56b01bf50b4cfe1ace9454dde9cdcaec0d475fb30c`

## Goal

Begin the standards/compatibility program needed to move Trinity Navigator from a bounded HTML/TLS viewer toward a general web browser. P1 deliberately strengthens the shared HTTP session contract instead of adding another site-specific workaround.

## Implemented

- Added a bounded Ring-3 cookie jar (16 entries) to Browser state.
- Parses multiple `Set-Cookie` response headers before redirects and resource follow-up.
- Supports host-only/domain cookies, path matching, `Secure`, and `Max-Age=0` removal.
- Sends matching cookies with documents, CSS, scripts, images, and media-range requests.
- Sends bounded `Referer` values for subresources.
- Added bounded `Cookie`/`Referer` fields to the Browser network syscall request.
- Added `Set-Cookie` and `Content-Encoding` response metadata to `FetchStatus`.
- Kernel HTTP construction rejects CR/LF in Browser-supplied header values.
- Cookie ownership stays in Ring-3; the kernel does not maintain a web-session database.
- Public-web JavaScript remains disabled in the compatibility scanner. P1 does not weaken the existing containment rule.

## Verification

The modified translation units were compiled individually with the production freestanding flags (`clang++`, `x86_64-pc-windows-msvc`, C++20, `-Wall -Wextra -Werror`):

- `src/kernel/netstack.cpp`
- `src/kernel/network.cpp`
- `src/kernel/syscall.cpp`
- `src/user/apps/browser.cpp`

Result: PASS.

`tests/browser_compat_p1_static.py`: PASS (10 checks).

A full EFI build was attempted but the frozen source package does not contain `third_party/libwebp/libwebpdecoder_upstream_1_5_0.a`; the existing build script stops on that missing baseline dependency before the final link. This P1 change did not introduce that missing dependency.

## Why this matters

A large portion of the modern web assumes stateful HTTP sessions even before JavaScript executes. Cookies and referrer-aware resource requests are prerequisites for authentication, consent/session state, CDN behavior, anti-CSRF flows, and many multi-request application bootstraps.

## Major blockers still standing between Trinity and broad modern-web compatibility

P1 is foundational; it does **not** make a 95% compatibility claim. The main remaining architectural gaps are:

1. Broader TLS interoperability (especially ECDSA certificate chains and additional TLS 1.3/1.2 cipher/signature profiles).
2. HTTP content coding (`gzip`, `deflate`, then Brotli) and caching semantics.
3. A real DOM and substantially broader HTML5 parsing.
4. A CSS cascade/layout engine rather than the current bounded renderer.
5. A real ECMAScript engine/event loop; the current compatibility runtime is intentionally not a general JS VM.
6. Fetch/XHR, timers, events, DOM mutation, storage, URL/history and Web APIs.
7. Modern image/font support and scalable resource/cache ownership.
8. Video/audio decode, MP4/WebM demux, streaming, buffering, A/V sync and Media Source-style behavior.
9. YouTube-specific modern playback requirements after the general JS/DOM/media foundations exist.

## Recommended next pass

P2 should implement bounded HTTP `gzip`/`deflate` decoding plus cache validators/metadata, then move immediately into TLS certificate/cipher expansion. These are low-level compatibility improvements shared by essentially every site and do not require weakening Browser process isolation.
