from pathlib import Path
h=Path('include/trinity/web_runtime.hpp').read_text()
w=Path('src/user/web_runtime.cpp').read_text()
b=Path('src/user/apps/browser.cpp').read_text()
checks=[
 'struct DomDocument' in h,
 'MaxNodes=768' in h,
 'MaxAttributes=1024' in h,
 'bool build_dom' in h,
 'dom_raw_name' in w,
 'dom_void_name' in w,
 'dom_attribute' in w,
 'dom_text_content' in w,
 'static web::DomDocument g_browser_dom' in b,
 'web::build_dom(st.page,st.page_len,g_browser_dom)' in b,
 'web::dom_find_first(g_browser_dom,"title",1)' in b,
 'g_browser_dom_valid=false' in b,
]
print(f"browser_compat_p4_static: {'PASS' if all(checks) else 'FAIL'} ({sum(checks)}/{len(checks)} checks)")
raise SystemExit(0 if all(checks) else 1)
