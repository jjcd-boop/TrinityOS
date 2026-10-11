#include "trinity/web_runtime.hpp"
#include <cstdio>
#include <cstring>
using namespace trinity;
int main(){
 const char* h="<!doctype html><html><head><title>Trinity &amp; Web</title><meta charset='utf-8'></head><body><main id=app data-x=\"7\"><p>Hello <b>world</b>.</p><img src='/a.png'><input disabled></main><script>const x='<tag>'; window.x=x;</script></body></html>";
 web::DomDocument d{}; bool ok=web::build_dom(h,std::strlen(h),d);
 int pass=0,total=10;
 if(ok)++pass;
 auto title=web::dom_find_first(d,"title",1); if(title!=0xffffu)++pass;
 char text[96]{}; auto tn=title==0xffffu?0:web::dom_text_content(d,title,text,sizeof(text)); if(tn==13&&std::strcmp(text,"Trinity & Web")==0)++pass;
 auto mainn=web::dom_find_first(d,"main",1); if(mainn!=0xffffu)++pass;
 web::DomSlice v{}; if(mainn!=0xffffu&&web::dom_attribute(d,mainn,"id",v)&&v.bytes==3&&std::memcmp(v.data,"app",3)==0)++pass;
 if(mainn!=0xffffu&&web::dom_attribute(d,mainn,"data-x",v)&&v.bytes==1&&v.data[0]=='7')++pass;
 auto img=web::dom_find_first(d,"img",1); if(img!=0xffffu&&web::dom_attribute(d,img,"src",v)&&v.bytes==6)++pass;
 auto input=web::dom_find_first(d,"input",1); if(input!=0xffffu&&web::dom_attribute(d,input,"disabled",v)&&v.bytes==0)++pass;
 auto script=web::dom_find_first(d,"script",1); char raw[96]{}; if(script!=0xffffu&&web::dom_text_content(d,script,raw,sizeof(raw))>0&&std::strstr(raw,"<tag>"))++pass;
 if(!d.node_overflow&&!d.attribute_overflow&&d.node_count>10)++pass;
 std::printf("browser p4 dom: %d/%d nodes=%u attrs=%u title=%s\n",pass,total,d.node_count,d.attribute_count,text);
 return pass==total?0:1;
}
