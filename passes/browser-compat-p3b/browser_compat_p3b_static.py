from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
checks=[]
def need(rel,text,label):
    s=(ROOT/rel).read_text(errors='ignore')
    ok=text in s
    checks.append((ok,label))
    if not ok: print('FAIL:',label)
need('src/kernel/tls.cpp','OID_ECDSA_SHA256','ECDSA-SHA256 X.509 signature OID')
need('src/kernel/tls.cpp','OID_P256','P-256 SPKI curve OID')
need('src/kernel/tls.cpp','ecdsa_p256_sha256','freestanding P-256 ECDSA verifier')
need('src/kernel/tls.cpp','p256_j_double','Jacobian P-256 scalar arithmetic')
need('src/kernel/tls.cpp','p256_shared_x','P-256 ECDH shared-secret path')
need('src/kernel/tls.cpp','put16(h+p,0x0017)','P-256 advertised in supported groups')
need('src/kernel/tls.cpp','sigalg==0x0403','TLS 1.2 ECDSA ServerKeyExchange verification')
need('src/kernel/tls.cpp','alg!=0x0804&&alg!=0x0403','TLS 1.3 RSA-PSS/ECDSA CertificateVerify acceptance')
need('src/kernel/tls.cpp','cipher!=0xc02f&&cipher!=0xc02b','TLS 1.2 ECDHE_RSA + ECDHE_ECDSA suites')
need('src/kernel/tls.cpp','tls12_group_=group','TLS 1.2 negotiated ECDHE group retention')
need('src/kernel/tls.cpp','copy(ckx+5,p256_public_,65)','P-256 ClientKeyExchange emission')
need('src/kernel/tls.cpp','if(child.sig_kind==3)return ecdsa_p256_sha256','ECDSA certificate-chain signatures')
need('src/kernel/tls.cpp','trinity_tls_test_p256_shared','P-256 runtime verification hook')
need('include/trinity/tls.hpp','p256_peer_[65]','bounded TLS P-256 peer point storage')
if not all(ok for ok,_ in checks): raise SystemExit(1)
print(f'PASS: Browser compatibility P3B static contract ({len(checks)} checks)')
