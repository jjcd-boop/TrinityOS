from pathlib import Path

tls = Path('src/kernel/tls.cpp').read_text()
net = Path('src/kernel/netstack.cpp').read_text()
br = Path('src/user/apps/browser.cpp').read_text()
hdr = Path('include/trinity/tls.hpp').read_text()
checks = [
    ('TLS fallback SCSV', '0x00ff' in tls),
    ('signature_algorithms_cert extension', 'signature_algorithms_cert' in tls and '0x0032' in tls),
    ('TLS 1.2 EMS advertised', 'put16(h+p,0x0017)' in tls),
    ('TLS 1.2 EMS parsed', 'tls12_extended_master_secret_' in tls and 'parse_extensions(b+p,en,0x0017' in tls),
    ('TLS 1.2 EMS key derivation', '"extended master secret"' in tls and 'transcript_hash(session_hash)' in tls),
    ('fetch metadata', 'Sec-Fetch-Site:' in net and 'Sec-Fetch-Dest:' in net and 'Sec-Fetch-Mode:' in net),
    ('POST origin', 'Origin: ' in net and 'origin_from_referer' in net),
    ('SameSite cookies', 'samesite' in br.lower() and 'same_site' in br),
    ('cookie prefixes', '__Secure-' in br and '__Host-' in br),
    ('Brotli not falsely advertised', 'Accept-Encoding: gzip, deflate\\r\\n' in net and 'Accept-Encoding: gzip, deflate, br' not in net),
]
failed = [name for name, ok in checks if not ok]
if failed:
    raise SystemExit('FAIL: ' + ', '.join(failed))
print(f'PASS: Browser compatibility P3A static contract ({len(checks)} checks)')
