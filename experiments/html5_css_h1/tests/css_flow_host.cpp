#include "trinity_css_flow.hpp"
#include <cstdio>
using namespace trinity::css;
static int fails=0;
static void check(bool pass,const char* what) {
    if(!pass){std::printf("FAIL %s\n",what);++fails;}
}
int main() {
    {
        Flow flow(800,600); LayoutBox outer{},inner{},closed{};
        Style a{};a.width=500;a.border_box=true;a.padding_left=20;a.padding_right=20;
        a.padding_top=10;a.padding_bottom=12;a.border=2;a.margin_top=7;
        check(flow.open(1,a,outer),"open parent");
        check(outer.border.w==500 && outer.content.w==456,"border box");
        check(outer.content.x==22 && outer.content.y==19,"parent positioning");
        Style b{};b.percent_width=5000;b.padding_left=10;b.padding_right=10;
        check(flow.open(2,b,inner),"nested percentage");
        check(inner.border.w==248 && inner.content.w==228,"percentage sizing");
        Rect first{};
        check(flow.inline_item(200,16,first),"first inline");
        check(first.x==32 && first.y==19,"first inline positioned");
        Rect second{};
        check(flow.inline_item(90,18,second),"second inline");
        check(second.y==35 && second.x==32,"inline wraps inside nested container");
        check(flow.close(closed) && closed.content.h==34,"nested height");
        check(flow.close(closed) && closed.border.h==60,"parent height respects padding/border");
        check(flow.depth()==1,"depth restored");
        check(!flow.close(closed),"cannot pop root");
    }
    {
        Flow flow(100,300); LayoutBox b{};Style s{};
        check(flow.open(10,s,b),"text container");
        Rect first{};uint32_t n=0;
        check(flow.text_run(25,10,12,first,n),"wrapped text");
        check(first.x==0 && first.y==0 && n==3,"text fragmentation 10/10/5");
        check(flow.close(b) && b.border.h==36,"text block three lines tall");
    }
    {
        Flow flow(100,100); LayoutBox b{};Style s{};
        bool ok=true;for(int i=0;i<31;++i)ok=ok&&flow.open(i+1,s,b);
        check(ok && flow.depth()==32,"bounded depth");
        check(!flow.open(33,s,b),"stack overflow rejected");
    }
    {
        Flow flow(200,200); LayoutBox b{};Style s{};s.width=500;
        check(flow.open(1,s,b) && b.overflow,"overflow reported");
    }
    return fails?1:(std::puts("PASS CSS flow H1 host tests"),0);
}
