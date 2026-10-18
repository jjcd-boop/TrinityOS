#include "trinity_html5_tree.hpp"
#include <cstdio>
#include <cstring>
#include <string>
using namespace trinity::html5;

static TreeBuilder tree;
static constexpr uint16_t NO=TreeNode::None;
static int failures=0;

static void check(bool yes,const char* desc){
    if(!yes){++failures;std::printf("FAIL %s\n",desc);}
}
static bool eq(const char* a,const char* b){
    return a && b && std::strcmp(a,b)==0;
}
static uint16_t first(uint16_t node){
    const auto* n=tree.get(node);return n?n->first_child:NO;
}
static uint16_t next(uint16_t node){
    const auto* n=tree.get(node);return n?n->next_sibling:NO;
}
static uint16_t child(uint16_t node,const char* name){
    return tree.first_named_child(node,name);
}
static bool tag(uint16_t node,const char* name){
    const auto* n=tree.get(node);
    return n && n->kind==NodeKind::Element && eq(n->name,name);
}
static bool text(uint16_t node,const char* value){
    const auto* n=tree.get(node);
    return n && n->kind==NodeKind::Text && eq(n->text,value);
}
static void parse(const char* src){
    if(!tree.build(src,std::strlen(src))){
        ++failures;
        std::printf("FAIL parser returned %d for %s\n",int(tree.error()),src);
    }
}
static bool linked_graph(){
    const size_t n=tree.count();
    if(n<4||n>TreeBuilder::kMaxNodes)return false;
    uint16_t reached[TreeBuilder::kMaxNodes]{};
    for(size_t i=0;i<n;++i){
        const auto* node=tree.get(static_cast<uint16_t>(i));
        if(!node)return false;
        if(i==0 && node->parent!=NO)return false;
        if(i>0 && (node->parent==NO || node->parent>=n))return false;
        uint16_t p=node->first_child,last=NO;
        for(size_t steps=0;p!=NO;++steps){
            if(steps>=n||p>=n)return false;
            const auto* c=tree.get(p);
            if(c->parent!=i || ++reached[p]!=1)return false;
            last=p;p=c->next_sibling;
        }
        if(node->last_child!=last)return false;
        size_t hops=0;
        for(uint16_t a=static_cast<uint16_t>(i);a!=NO;a=tree.get(a)->parent)
            if(++hops>n)return false;
    }
    for(size_t i=1;i<n;++i)if(reached[i]!=1)return false;
    return true;
}
int main(){
    {
        parse("<table><b><tr><td>A</td></tr>B</table>C");
        const uint16_t body=tree.body_index();
        const uint16_t empty_b=first(body);
        const uint16_t text_b=next(empty_b);
        const uint16_t table_node=next(text_b);
        const uint16_t trailing_b=next(table_node);
        check(tag(empty_b,"b") && first(empty_b)==NO,"misplaced b exists before table");
        check(tag(text_b,"b") && text(first(text_b),"B"),"active b restored before table");
        check(tag(table_node,"table"),"table follows fostered b");
        const uint16_t td=child(child(child(table_node,"tbody"),"tr"),"td");
        check(text(first(td),"A"),"table cell preserved");
        check(tag(trailing_b,"b") && text(first(trailing_b),"C"),"b reconstructed after table");
        check(next(trailing_b)==NO,"no extra body sibling");
        check(linked_graph(),"table formatting repairs preserve graph links");
    }
    {
        parse("<table><tr><td><b>A<table><tr><td>B</b>C</td></tr></table></td></tr></table>");
        const uint16_t outer=child(tree.body_index(),"table");
        const uint16_t outer_cell=child(child(child(outer,"tbody"),"tr"),"td");
        const uint16_t bold=child(outer_cell,"b");
        const uint16_t inner=child(bold,"table");
        const uint16_t inner_cell=child(child(child(inner,"tbody"),"tr"),"td");
        check(tag(outer,"table")&&tag(bold,"b")&&tag(inner,"table"),"nested table structure");
        check(text(first(bold),"A"),"outer cell text before nested table");
        check(text(first(inner_cell),"BC"),"out-of-scope closing b keeps BC in inner td");
        check(next(first(inner_cell))==NO,"adjacent inner cell text coalesced");
        check(linked_graph(),"nested-table formatting maintains parent links");
    }
    {
        parse("<table><tr><td><b>A</b></td><td>B</td></tr></table>");
        const uint16_t table_node=child(tree.body_index(),"table");
        const uint16_t tr=child(child(table_node,"tbody"),"tr");
        const uint16_t td=child(tr,"td");
        check(tag(td,"td") && tag(child(td,"b"),"b"),"valid first cell formatting");
        check(tag(next(td),"td") && text(first(next(td)),"B"),"second cell not corrupted");
        check(linked_graph(),"normal table context regression");
    }
    {
        parse("<table><tbody><tr><td>one</td></tr></tbody></table>");
        const uint16_t table_node=child(tree.body_index(),"table");
        const uint16_t group=child(table_node,"tbody");
        const uint16_t tr=child(group,"tr");
        check(tag(group,"tbody") && !tree.get(group)->implied,"explicit tbody preserved");
        check(text(first(child(tr,"td")),"one"),"explicit group text");
        check(linked_graph(),"explicit group DOM links");
    }
    {
        parse("<table><b><tbody><tr><td>A</td></tr></tbody></table>");
        const uint16_t body=tree.body_index();
        const uint16_t b=first(body),table_node=next(b);
        check(tag(b,"b") && tag(table_node,"table"),"misplaced formatting before explicit tbody");
        check(tag(child(table_node,"tbody"),"tbody"),"explicit tbody under table");
        check(text(first(child(child(child(table_node,"tbody"),"tr"),"td")),"A"),
              "cell survives formatting cleanup");
        check(linked_graph(),"explicit group after misnested formatting");
    }
    {
        parse("<table><tr><td><table><tbody><tr><td>X</td></tr></tbody></table></td></tr></table>");
        const uint16_t outer=child(tree.body_index(),"table");
        const uint16_t td=child(child(child(outer,"tbody"),"tr"),"td");
        const uint16_t inner=child(td,"table");
        check(tag(inner,"table") && tag(child(inner,"tbody"),"tbody"),"nested valid tables");
        check(text(first(child(child(child(inner,"tbody"),"tr"),"td")),"X"),"nested cell content");
        check(linked_graph(),"nested explicit groups remain coherent");
    }
    {
        // Deterministic malformed combinations with a check of every link
        // and ancestor chain, including failures with an explicit error.
        const char* tags[]={
            "<table>","<tbody>","<thead>","<tfoot>","<tr>","<td>","<th>",
            "<b>","<i>","<em>","<div>","<p>","<span>",
            "</span>","</p>","</div>","</b>","</i>","</em>",
            "</td>","</th>","</tr>","</tbody>","</thead>",
            "</tfoot>","</table>","A"," B"
        };
        uint32_t state=0x8f3a917du;
        for(size_t trial=0;trial<400;++trial){
            std::string html="<body>";
            for(size_t i=0;i<65;++i){
                state=state*1664525u+1013904223u;
                html+=tags[(state>>16)%(sizeof(tags)/sizeof(tags[0]))];
            }
            bool ok=tree.build(html.data(),html.size());
            if(ok)check(linked_graph(),"malformed table ownership invariant");
            else check(tree.error()!=BuildError::None,"malformed table failure status");
        }
    }
    if(failures){
        std::printf("FAIL H3C1 table context and scope (%d failures)\n",failures);
        return 1;
    }
    std::puts("PASS H3C1 table context, nested formatting and DOM invariants");
    return 0;
}
