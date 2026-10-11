# Trinity OS Browser Compatibility P3B

Baseline: Browser Compatibility P3A / VM Pass 3C-R2 production freeze.

## Goal

Remove the RSA-only public-web TLS/X.509 limitation without weakening Trinity's trust checks and broaden TLS 1.2 ECDHE interoperability for modern ECDSA endpoints.

## Implemented

- X.509 SubjectPublicKeyInfo parsing for `id-ecPublicKey` on `prime256v1` / secp256r1.
- `ecdsa-with-SHA256` certificate-signature recognition and verification.
- Fixed-size freestanding P-256 arithmetic using Jacobian scalar operations to avoid inversion-per-step stalls.
- ECDSA P-256 SHA-256 (`0x0403`) in TLS `signature_algorithms` and `signature_algorithms_cert`.
- TLS 1.3 CertificateVerify accepts RSA-PSS-SHA256 or ECDSA-P256-SHA256 according to the leaf key.
- TLS 1.2 `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256` (`0xC02B`) alongside the existing ECDHE_RSA profile (`0xC02F`).
- TLS 1.2 ECDHE accepts X25519 or P-256; uncompressed P-256 peer points are curve-validated before ECDH.
- Deterministic per-connection P-256 ephemeral scalar/public-key derivation from Trinity entropy, separate from X25519.
- ClientKeyExchange generation for both 32-byte X25519 and 65-byte uncompressed P-256 points.
- Trust-anchor key identity supports both RSA and P-256 EC keys.
- Corrected the packaged P1/P2 static-contract root path so those regressions execute from the source tree.

## Verification

- P1 static contract: PASS (10/10).
- P2 static contract: PASS (10/10).
- P3B static contract: PASS (14/14).
- Raw generated P-256 ECDSA/SHA-256 signature vector: PASS.
- Generated ECDSA root -> leaf X.509 chain plus SAN hostname validation: PASS.
- Independent P-256 public-key derivation vector: PASS.
- Independent P-256 ECDH shared-secret vector: PASS.
- ECDSA host verification hot path: about 0.14 s on the build host after Jacobian optimization (down from about 5.4 s for the initial affine prototype).
- `tls.cpp`, `netstack.cpp`, and `browser.cpp` compile with Trinity's strict x86-64 freestanding `-Wall -Wextra -Werror` profile.

## Brotli status

Brotli is intentionally still **not advertised**. The installed host archive imports hosted libc and stack-protector/TLS ABI assumptions, so it is not a safe Trinity Ring-3 dependency. Official Brotli source is MIT licensed and remains the intended source-vendored route. Until that decoder is integrated with a bounded Trinity allocator and tested, HTTP truthfully advertises only `gzip, deflate`.

## Next browser step

Finish the source-vendored Brotli decoder as a contained Ring-3 component. After that, freeze transport work temporarily and move the main compatibility effort into DOM, layout/CSS, JavaScript/event-loop, fetch/XHR, and media architecture—the dominant blockers for broad modern-site and YouTube compatibility.
