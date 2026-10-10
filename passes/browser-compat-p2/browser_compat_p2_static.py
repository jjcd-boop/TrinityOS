from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
checks=[]
def need(rel,text,label):
    s=(ROOT/rel).read_text()
    assert text in s, f"missing {label}"
    checks.append(label)
need("src/kernel/netstack.cpp","Accept-Encoding: gzip, deflate","gzip/deflate negotiation")
need("src/kernel/netstack.cpp","If-None-Match: ","ETag conditional request")
need("src/kernel/netstack.cpp","If-Modified-Since: ","Last-Modified conditional request")
need("include/trinity/network.hpp","char etag[96]{};","ETag response metadata")
need("include/trinity/network.hpp","char cache_control[128]{};","Cache-Control response metadata")
need("src/kernel/network.cpp","last-modified:","Last-Modified parser")
need("src/user/apps/browser.cpp","browser_cache_restore","304 cache reuse")
need("src/user/apps/browser.cpp","browser_cache_store","bounded Ring-3 entity cache")
need("src/user/apps/browser.cpp","decode_http_content","Ring-3 HTTP decompression")
need("src/kernel/netstack.cpp","Accept-Encoding: identity","identity media range semantics")
print(f"browser_compat_p2_static: PASS ({len(checks)} checks)")
