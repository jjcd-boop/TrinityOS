# Browser Compatibility P3B

P3B adds ECDSA P-256 X.509 and TLS handshake interoperability, including TLS 1.2 P-256 ECDHE/ECDSA and TLS 1.3 ECDSA CertificateVerify.

Verification:

```sh
python tests/browser_compat_p1_static.py
python tests/browser_compat_p2_static.py
python tests/browser_compat_p3a_static.py
python tests/browser_compat_p3b_static.py
g++ -std=c++20 -O2 -DTRINITY_TLS_TEST_HOOKS -Iinclude src/kernel/tls.cpp tests/browser_compat_p3b_ecdsa.cpp -o /tmp/p3b && /tmp/p3b
```

See `BROWSER_COMPAT_P3B_REPORT.md` for scope, qualification, and the Brotli deferral rationale.
