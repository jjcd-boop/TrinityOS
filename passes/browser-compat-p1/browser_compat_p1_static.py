from pathlib import Path
root=Path(__file__).resolve().parents[1]
checks=[]
def need(path,text,label):
    data=(root/path).read_text(errors="ignore")
    ok=text in data
    checks.append((ok,label))
    if not ok: print("FAIL:",label)
need(Path("include/trinity/network.hpp"),"char set_cookie[512]{};","bounded Set-Cookie response metadata")
need(Path("include/trinity/syscall.hpp"),"char cookie[320];","bounded Ring-3 Cookie request field")
need(Path("include/trinity/syscall.hpp"),"char referer[384];","bounded Ring-3 Referer request field")
need(Path("src/kernel/netstack.cpp"),"safe_header_value(options.cookie)","kernel HTTP header injection guard")
need(Path("src/kernel/netstack.cpp"),"Cookie: ","HTTP Cookie emission")
need(Path("src/kernel/netstack.cpp"),"Referer: ","HTTP Referer emission")
need(Path("src/kernel/network.cpp"),"http_collect_header_values", "multiple Set-Cookie collection")
need(Path("src/user/apps/browser.cpp"),"BrowserCookie cookies[kBrowserCookieSlots]", "Ring-3 bounded cookie jar")
need(Path("src/user/apps/browser.cpp"),"browser_store_response_cookies(st,q->status.final_url,q->status.set_cookie);", "cookies committed before redirect/resource follow-up")
need(Path("src/user/apps/browser.cpp"),"browser_apply_request_context(st,r,st.media.stream_url,st.media.stream_url_len,true);", "media fetch receives web session context")
if not all(ok for ok,_ in checks): raise SystemExit(1)
print(f"PASS: Browser compatibility P1 static contract ({len(checks)} checks)")
