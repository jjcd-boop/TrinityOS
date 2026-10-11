from pathlib import Path

tls = Path('src/kernel/tls.cpp').read_text()
hdr = Path('include/trinity/tls.hpp').read_text()
net = Path('src/kernel/netstack.cpp').read_text()
checks = [
    ('ECDSA SHA-256 X.509 OID', 'OID_ECDSA_SHA256' in tls),
    ('EC public-key and prime256v1 OIDs', 'OID_EC_PUBLIC_KEY' in tls and 'OID_PRIME256V1' in tls),
    ('P-256 SPKI parser', 'c.key_kind=2' in tls and 'bits.n!=66' in tls),
    ('ECDSA P-256 verifier', 'ecdsa_p256_sha256' in tls),
    ('TLS signature scheme ECDSA P-256 SHA-256', '0x0403' in tls),
    ('TLS 1.2 ECDHE_ECDSA AES128 GCM suite', '0xc02b' in tls),
    ('TLS 1.2 P-256 ECDHE group', 'tls12_kx_group_' in tls and '0x0017' in tls),
    ('TLS 1.3 key-type matched CertificateVerify', 'SERVER SIGNATURE SCHEME DOES NOT MATCH CERTIFICATE KEY' in tls),
    ('leaf EC verification material retained', 'leaf_ec_x_' in hdr and 'leaf_ec_y_' in hdr),
    ('Brotli still not falsely advertised', 'Accept-Encoding: gzip, deflate\\r\\n' in net and 'Accept-Encoding: gzip, deflate, br' not in net),
]
failed=[name for name,ok in checks if not ok]
if failed:
    raise SystemExit('FAIL: '+', '.join(failed))
print(f'PASS: Browser compatibility P3B static contract ({len(checks)} checks)')
