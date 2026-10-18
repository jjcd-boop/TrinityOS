#include "trinity_css_c2.hpp"
#include "trinity_html5_segmented_nodes.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstddef>

using namespace trinity::css_c2;
using namespace trinity::css_c1;
using namespace trinity::html5;
static int fails=0;
static void check(bool x,const char* name){
    if(!x){++fails;std::printf("FAIL %s\n",name);}
}
static bool equal(const char* a,const char* b){return a&&b&&!std::strcmp(a,b);}
struct Allocator {size_t live{0};};
static void* allocate(size_t bytes,size_t align,void* c) noexcept{
    if(align>alignof(std::max_align_t))return nullptr;
    void* p=std::malloc(bytes);
    if(p)static_cast<Allocator*>(c)->live+=bytes;
    return p;
}
static void release(void* p,size_t bytes,size_t align,void* c) noexcept{
    (void)align;
    if(p)static_cast<Allocator*>(c)->live-=bytes;
    std::free(p);
}
static Stylesheet css;
static Allocator allocator;
static SegmentedNodeArena arena(NodeArenaMemory{&allocator,allocate,release},
                               1000,SegmentedNodeArena::segment_bytes()*150);
static TreeBuilder tree(arena);
static uint16_t direct(uint16_t parent,const char* tag){
    return tree.first_named_child(parent,tag);
}
int main(){
    {
        SelectorList selectors{};
        const char* s="main#app.shell > section[data-kind='news'] p.note.urgent, h1";
        check(selectors.parse(s,std::strlen(s)),"CSS selector list parses");
        check(selectors.count()==2,"selector groups");
        const Selector* first=selectors.get(0);
        check(first&&first->count==3,"three compounds");
        check(first&&first->specificity.ids==1&&
              first->specificity.classes==4&&first->specificity.elements==3,
              "specificity id, class, attribute and elements");
        check(first&&first->compounds[1].relation==Relation::Child&&
              first->compounds[2].relation==Relation::Descendant,
              "child and descendant combinators");
        check(selectors.get(1)&&selectors.get(1)->specificity.elements==1,
              "second selector group separate");
    }
    {
        SelectorList selectors{};
        const char* bad[]={"p:hover","div + p","div ~ p","p::before",
                           "a[href^=https]","div >",", p","p,","a:nth-child(2n)"};
        for(const char* s:bad)
            check(!selectors.parse(s,std::strlen(s)),
                  "unsupported/invalid selectors are rejected");
    }
    const char* html="<!doctype html><html><body>"
        "<main class='shell' id='app'>"
        "<section data-kind='news'><p id='lead' class='note urgent'>Hello</p></section>"
        "<p class='note'>Second</p><span>inherited</span>"
        "</main></body></html>";
    check(tree.build(html,std::strlen(html)),"parsed HTML document for CSS matching");
    uint16_t main_node=direct(tree.body_index(),"main");
    uint16_t section=direct(main_node,"section");
    uint16_t lead=direct(section,"p");
    uint16_t second=direct(main_node,"p");
    uint16_t span=direct(main_node,"span");
    check(main_node!=TreeNode::None&&section!=TreeNode::None&&
          lead!=TreeNode::None&&second!=TreeNode::None&&span!=TreeNode::None,
          "real DOM nodes accessible to CSS");
    {
        SelectorList list{};
        const char* s="#lead, main > section > p.urgent, main > p.note";
        check(list.parse(s,std::strlen(s)),"compound selector list");
        check(matches(*list.get(0),tree,lead) &&
              !matches(*list.get(0),tree,second),"id specificity matching");
        check(matches(*list.get(1),tree,lead) &&
              !matches(*list.get(1),tree,second),"child combinator matching");
        check(matches(*list.get(2),tree,second) &&
              !matches(*list.get(2),tree,lead),"parent-child selector semantics");
    }
    const char* stylesheet=":root { color: teal; font-size: 20px; }"
        "main { color: green; font-weight: 700; }"
        "p { color: black; display: block; }"
        ".note { color: blue; }"
        "#lead { color: red; }"
        "main .note { font-size: 18px; }"
        "main > section > p.urgent { background-color: yellow; }"
        "section[data-kind='news'] p { padding: 8px; }"
        "p { color: purple !important; }"
        "#lead { color: orange; }"
        "p:hover { color: pink !important; }";
    check(css.parse(stylesheet,std::strlen(stylesheet)),"CSS stylesheet parses");
    check(css.count()==11,"all 11 source rules retained");
    static SelectorList resolver_scratch;
    StyleResolver resolver(tree,css,resolver_scratch);
    ComputedStyle leadstyle{};
    check(resolver.compute(lead,leadstyle),"computed style for lead paragraph");
    check(equal(leadstyle.color,"purple"),"!important overrides ID");
    check(equal(leadstyle.background_color,"yellow"),
          "child selector background-color");
    check(equal(leadstyle.font_size,"18px"),"descendant font-size");
    check(equal(leadstyle.font_weight,"700"),"parent font weight inherited");
    check(equal(leadstyle.padding,"8px"),"attribute selector and descendant padding");
    check(equal(leadstyle.display,"block"),"display from qualified rule");
    ComputedStyle secondstyle{};
    check(resolver.compute(second,secondstyle),"compute second paragraph");
    check(equal(secondstyle.color,"purple"),"shared !important rule");
    check(equal(secondstyle.background_color,"transparent"),
          "nonmatching nested selector excluded");
    check(equal(secondstyle.font_size,"18px"),"descendant class matches");
    check(equal(secondstyle.padding,"0"),"noninherited padding");
    ComputedStyle sectionstyle{};
    check(resolver.compute(section,sectionstyle),"compute section inheritance");
    check(equal(sectionstyle.color,"green") &&
          equal(sectionstyle.font_size,"20px") &&
          equal(sectionstyle.font_weight,"700"),
          "parent/root inheritance propagated");
    ComputedStyle spanstyle{};
    check(resolver.compute(span,spanstyle),"compute span style");
    check(equal(spanstyle.color,"green") &&
          equal(spanstyle.background_color,"transparent") &&
          equal(spanstyle.font_weight,"700") &&
          equal(spanstyle.display,"inline"),"unstyled span inheritance");
    {
        const char* css2="p{color:red;} #lead{color:green;} .note{color:blue;}"
            "p{color:black!important} #lead{color:gold!important}";
        check(css.parse(css2,std::strlen(css2)),"specificity override stylesheet");
        check(resolver.compute(lead,leadstyle) &&
              equal(leadstyle.color,"gold"),"important ID outranks important tag");
        check(resolver.compute(second,secondstyle) &&
              equal(secondstyle.color,"black"),"important tag outranks ordinary class");
    }
    {
        const char* css3="p{margin:1px;margin:2px;color:black;color:navy}";
        check(css.parse(css3,std::strlen(css3)),"same-rule ordering parse");
        check(resolver.compute(second,secondstyle) &&
              equal(secondstyle.margin,"2px")&&equal(secondstyle.color,"navy"),
              "later declaration wins specificity tie");
    }
    {
        ComputedStyle invalid{};
        check(!resolver.compute(TreeNode::None,invalid) &&
              resolver.error()==ResolveError::InvalidNode,
              "invalid DOM handles are rejected");
        check(!resolver.compute(0,invalid) &&
              resolver.error()==ResolveError::InvalidNode,
              "document root not mistaken for HTML element");
    }
    arena.clear();
    check(allocator.live==0,"caller receives all segmented allocations back");
    if(fails){std::printf("FAIL Navigator C2/H4A %d tests\n",fails);return 1;}
    std::puts("PASS Navigator C2 specificity/selector/cascade/H4A read-only DOM styling");
    return 0;
}
