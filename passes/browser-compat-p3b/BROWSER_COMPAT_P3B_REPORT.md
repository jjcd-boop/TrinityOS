# Trinity OS Browser Compatibility P3B

Date: 2026-10-10
Baseline: Browser Compatibility P3A / VM Pass 3C-R2 production freeze

## Goal

Remove the RSA-only public-web TLS/X.509 limitation without weakening Trinity's trust checks. P3B adds P-256/ECDSA certificate and handshake support and completes the TLS 1.2 ECDHE profile needed by common ECDSA endpoints.

## Implemented

- X.509 SubjectPublicKeyInfo parsing for `id-ecPublicKey` on `prime256v1` / secp256r1.
- `ecdsa-with-SHA256` certificate-signature recognition and verification.
- Fixed-size, freestanding P-256 field/point arithmetic used only for public-key verification/ECDH.
- ECDSA P-256 SHA-256 (`0x0403`) in TLS signature negotiation.
- TLS 1.3 CertificateVerify accepts RSA-PSS-SHA256 for RSA leaves or ECDSA-P256-SHA256 for EC leaves and rejects mismatched key/signature profiles.
- TLS 1.2 `ECDHE_ECDSA_WITH_AES_128_GCM_SHA256` (`0xC02B`) in addition to the existing RSA profile (`0xC02F`).
- TLS 1.2 ECDHE accepts X25519 or P-256; P-256 peer points are validated before scalar multiplication.
- Deterministic P-256 ephemeral key derivation from Trinity's per-connection entropy seed, separate from the X25519 derivation.
- ClientKeyExchange generation for both 32-byte X25519 and uncompressed 65-byte P-256 public points.
- Trust-anchor matching supports RSA and EC public keys.
- EC verification material is retained only as public coordinates; no certificate private material is introduced.

## Verification

- P1 static contract: PASS (10/10)
- P2 static contract: PASS (10/10)
- P2 live gzip/zlib/raw-deflate decoder test: PASS
- P3A static contract: PASS (10/10)
- P3B static contract: PASS (10/10)
- P3B runtime ECDSA test: PASS
  - valid P-256/SHA-256 signature accepted
  - tampered hash rejected
  - self-signed P-256 X.509 certificate parsed and signature verified
  - Trinity P-256 public-key derivation matched an independent cryptography implementation
  - Trinity ECDH shared secret matched the independent implementation
- `tls.cpp`, `netstack.cpp`, `web_runtime.cpp`, and `browser.cpp` compile with the production x86-64 freestanding `-Wall -Wextra -Werror` profile.

## Full EFI build status

The frozen source package still omits `third_party/libwebp/libwebpdecoder_upstream_1_5_0.a`. `scripts/build.sh` stops on that pre-existing packaging dependency before linking. This is not caused by P3B.

## Brotli status

P3B does **not** falsely advertise `br`. Investigation confirmed that Google's decoder source is suitable to vendor under its MIT license, but a hosted Linux Brotli binary is not acceptable for Trinity's freestanding Ring-3 ABI. Until the decoder source and bounded allocator are integrated and tested in Trinity, HTTP continues to advertise `gzip, deflate` only.

This preserves correctness: a server will not send a Brotli entity Trinity cannot decode.

## Next browser step

Finish source-vendored Brotli as a contained Ring-3 decoder, then shift the main compatibility effort from transport into the substantially larger DOM/CSS/JavaScript execution architecture required for modern application sites and YouTube.
