# Trinity Browser Compatibility P3A

Baseline: `TrinityOS_BROWSER_COMPAT_P2_SOURCE.zip`

## Scope

P3A strengthens TLS 1.2 interoperability, browser request metadata, and cookie/session semantics. It intentionally does **not** advertise Brotli yet: host-distribution Brotli binaries were rejected because they carry a hosted Linux ABI / stack-protector dependency and are not suitable for Trinity's freestanding Ring-3 environment. Brotli will be integrated from auditable source as a separate freestanding decoder slice.

## Implemented

- TLS_FALLBACK_SCSV (`0x00ff`) in the offered cipher list.
- `signature_algorithms_cert` extension matching Trinity's actually implemented RSA verification algorithms.
- RFC 7627 Extended Master Secret advertisement, ServerHello negotiation detection, status telemetry, and TLS 1.2 master-secret derivation from the session hash when negotiated.
- Modern Fetch Metadata request headers (`Sec-Fetch-Site`, `Sec-Fetch-Dest`, `Sec-Fetch-Mode`, and document `Sec-Fetch-User`).
- `Origin` generation for form POST requests from the current document origin.
- POST referrer propagation so common origin/CSRF validation paths receive expected request context.
- Cookie default-path derivation and segment-boundary path matching.
- `HttpOnly` state retention, `SameSite=Lax/Strict/None` policy, `SameSite=None` + `Secure` enforcement, insecure-origin Secure-cookie rejection, and `__Secure-` / `__Host-` prefix enforcement.
- Brotli remains deliberately unadvertised (`Accept-Encoding: gzip, deflate`) until a freestanding source decoder is integrated and validated.

## Verification

- `src/kernel/tls.cpp` compiles with the production freestanding Windows-target flags and `-Wall -Wextra -Werror`.
- `src/kernel/netstack.cpp` compiles with the same strict flags.
- `src/user/apps/browser.cpp` compiles with the production standalone-app freestanding flags and `-Wall -Wextra -Werror`.
- P1 static contract: PASS (10 checks).
- P2 static contract: PASS (10 checks).
- P3A static contract: PASS (10 checks).

## Still open

- ECDSA X.509 public keys and ECDSA certificate signatures remain unsupported; the current certificate path is RSA-only.
- TLS cipher diversity remains narrow (TLS 1.3 AES-128-GCM-SHA256 and TLS 1.2 ECDHE-RSA-AES128-GCM-SHA256).
- Brotli source decoder integration remains pending.
- Full modern DOM/CSS/JavaScript compatibility remains the dominant requirement for the 95%-of-web / YouTube target.
