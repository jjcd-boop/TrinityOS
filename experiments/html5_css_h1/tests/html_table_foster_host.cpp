#include "trinity_html5_tree.hpp"
#include <cstdio>
#include <cstring>
#include <string>
using namespace trinity::html5;
static TreeBuilder document;
static int failures=0;
static constexpr uint16_t NO=TreeNode::None;

static bool eq(const char* a,const char* b) {
    return a && b && std::strcmp(a,b)==0;
}
static void check(bool yes,const char* what){
    if(!yes){++failures;std::printf("FAIL %s\n",what);}
}
static void parse(const char* src) {
    if(!document.build(src,std::strlen(src))){
        ++failures;std::printf("FAIL parse %s code=%d\n",src,int(document.error()));
    }
}
static uint16_t named(uint16_t p,const char* name){
    return document.first_named_child(p,name);
}
static uint16_t first(uint16_t i){
    const auto* p=document.get(i);return p?p->first_child:NO;
}
static uint16_t next(uint16_t i){
    const auto* p=document.get(i);return p?p->next_sibling:NO;
}
static bool is_text(uint16_t i,const char* value){
    const auto* p=document.get(i);
    return p && p->kind==NodeKind::Text && eq(p->text,value);
}
static bool validate_graph(){
    const size_t size=document.count();
    if(size<4||size>TreeBuilder::kMaxNodes)return false;
    size_t seen[TreeBuilder::kMaxNodes]{};
    for(size_t i=0;i<size;++i){
        auto* p=document.get(static_cast<uint16_t>(i));
        if(!p)return false;
        if(i==0 && p->parent!=NO)return false;
        if(i>0 && (p->parent==NO||p->parent>=size))return false;
        uint16_t c=p->first_child,last=NO;size_t guard=0;
        while(c!=NO){
            if(c>=size || ++guard>size)return false;
            const auto* child=document.get(c);
            if(child->parent!=i)return false;
            ++seen[c];last=c;c=child->next_sibling;
        }
        if(last!=p->last_child)return false;
    }
    for(size_t i=1;i<size;++i)if(seen[i]!=1)return false;
    return seen[0]==0;
}
int main(){
    {
        parse("<table>Hello<tr><td>cell</td></tr></table>");
        uint16_t text=first(document.body_index());
        uint16_t table=next(text);
        check(is_text(text,"Hello"),"nonwhitespace fostered outside table");
        check(table!=NO && eq(document.get(table)->name,"table"),
              "fostered text precedes table");
        uint16_t tbody=named(table,"tbody"),tr=named(tbody,"tr");
        check(tbody!=NO && tr!=NO && is_text(first(named(tr,"td")),"cell"),
              "table structure preserved after foster");
        check(validate_graph(),"fostered text links maintain tree invariants");
    }
    {
        parse("<table>  <tbody><tr><td>x</td></tr></tbody></table>");
        uint16_t table=named(document.body_index(),"table");
        check(is_text(first(table),"  "),"pure table whitespace stays inside");
        check(next(first(table))==named(table,"tbody"),"tbody follows table whitespace");
        check(validate_graph(),"whitespace graph invariant");
    }
    {
        parse("<div>before<table><p>outside</p><tr><td>inside</td></tr></table>after</div>");
        uint16_t div=named(document.body_index(),"div");
        uint16_t before=first(div),p=next(before),table=next(p),after=next(table);
        check(is_text(before,"before"),"existing sibling before foster");
        check(p!=NO && eq(document.get(p)->name,"p") &&
              is_text(first(p),"outside"),"paragraph fostered outside table");
        check(table!=NO && eq(document.get(table)->name,"table"),"table remains in div");
        check(is_text(after,"after"),"following text retained");
        check(validate_graph(),"fostered element re-linking valid");
    }
    {
        parse("<table>one <b>two</b> three<tr><td>cell</td></tr></table>");
        uint16_t a=first(document.body_index()),b=next(a);
        uint16_t c=next(b),table=next(c);
        check(is_text(a,"one "),"foster first text");
        check(b!=NO && eq(document.get(b)->name,"b") &&
              is_text(first(b),"two"),"foster formatting element");
        check(is_text(c," three"),"foster trailing text");
        check(table!=NO && eq(document.get(table)->name,"table"),"foster order before table");
        check(validate_graph(),"fostered element ordering invariant");
    }
    {
        parse("<table><tbody>outside<tr><td>inside</td></tr></tbody></table>");
        uint16_t text=first(document.body_index()),table=next(text);
        check(is_text(text,"outside"),"tbody nonwhitespace foster");
        check(table!=NO && eq(document.get(table)->name,"table"),"tbody table follows foster");
        uint16_t td=named(named(named(table,"tbody"),"tr"),"td");
        check(is_text(first(td),"inside"),"inside cell not fostered");
        check(validate_graph(),"tbody graph invariant");
    }
    {
        parse("<table><caption>caption</caption><tr><td>cell</td></tr></table>");
        uint16_t table=named(document.body_index(),"table");
        check(is_text(first(named(table,"caption")),"caption"),
              "caption text remains within table");
        check(validate_graph(),"caption graph invariant");
    }
    {
        parse("<table><tr><td><table>inner<tr><td>nested</td></tr></table></td></tr></table>");
        uint16_t outer=named(document.body_index(),"table");
        uint16_t cell=named(named(named(outer,"tbody"),"tr"),"td");
        uint16_t text=first(cell),inner=next(text);
        check(is_text(text,"inner"),"nested table foster stays inside its cell");
        check(inner!=NO && eq(document.get(inner)->name,"table"),"inner table preserved");
        check(is_text(first(named(named(named(inner,"tbody"),"tr"),"td")),"nested"),
              "nested cell contents retained");
        check(validate_graph(),"nested foster ownership graph invariant");
    }
    {
        parse("<table>abc<!--hi--><tr><td>cell</td></tr></table>");
        uint16_t text=first(document.body_index());
        uint16_t table=next(text);
        check(is_text(text,"abc"),"comment adjacent fostered text");
        check(table!=NO && eq(document.get(table)->name,"table"),"table after text");
        check(document.get(first(table))->kind==NodeKind::Comment,"comment retained in table");
        check(validate_graph(),"comment graph invariant");
    }
    {
        std::string long_text="<table>";
        long_text+=std::string(780,'Q');
        long_text+="<tr><td>Z</td></tr></table>";
        const bool pass=document.build(long_text.data(),long_text.size());
        check(pass,"large foster text within node budget");
        if(pass){
            uint16_t n=first(document.body_index());
            size_t count=0;bool text_only=true;
            while(n!=NO && document.get(n)->kind==NodeKind::Text){
                count+=document.get(n)->text_len;
                for(unsigned k=0;k<document.get(n)->text_len;++k)
                    if(document.get(n)->text[k]!='Q')text_only=false;
                n=next(n);
            }
            check(count==780 && text_only,"large foster text not truncated");
            check(n!=NO && eq(document.get(n)->name,"table"),"large foster precedes table");
            check(validate_graph(),"long foster graph invariant");
        }
    }
    {
        // Deterministic malformed HTML samples exercise iterative insertion and
        // invariant checks without heap allocation inside the tree builder.
        static const char* specimens[]={
            "<table><tbody><tr>x<td>y</table>",
            "<table><div><p>outside</p></div><tbody><tr><td>inside",
            "<table><tr><img src=x><td>A<td>B",
            "<p>hello<table>world</table>end",
            "<div><table><tr><td>A</td>outside</tr></table></div>",
            "<table>foo<table>bar</table>baz</table>",
            "<table><thead>head<tr><td>data</td></tr></thead></table>",
            "<table><tbody><tr><td>one</td></tr>tail</tbody></table>"
        };
        for(const char* specimen:specimens){
            if(document.build(specimen,std::strlen(specimen)))
                check(validate_graph(),"malformed table graph integrity");
            else check(document.error()!=BuildError::None,
                       "malformed table failure has explicit error");
        }
    }
    {
        // Deterministic malformed-markup stress: verify all accepted trees
        // remain linked and every rejected tree reports an explicit bound/error.
        const char* tags[]={"<table>","<tbody>","<tr>","<td>","<div>","<p>",
                            "<b>","</b>","</div>","</p>","</td>","</tr>",
                            "</tbody>","</table>","A"," B","<img src=x>"};
        unsigned int state=0x9e3779b9u;
        for(unsigned round=0;round<300;++round) {
            std::string sample="<body>";
            for(unsigned step=0;step<56;++step) {
                state=state*1664525u+1013904223u;
                sample+=tags[(state>>16)%(sizeof(tags)/sizeof(tags[0]))];
            }
            const bool success=document.build(sample.data(),sample.size());
            if(success)check(validate_graph(),"stress graph invariants");
            else check(document.error()!=BuildError::None,
                       "stress rejects with documented error");
        }
    }
    if(failures) {
        std::printf("FAIL H3A table foster suite (%d failures)\n",failures);
        return 1;
    }
    std::puts("PASS H3A bounded HTML table foster parenting host tests");
    return 0;
}
