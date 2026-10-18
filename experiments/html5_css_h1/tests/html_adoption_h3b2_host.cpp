#include "trinity_html5_tree.hpp"
#include <cstdio>
#include <cstring>
#include <string>

using namespace trinity::html5;
static TreeBuilder doc;
static int fails=0;
static constexpr uint16_t NO=TreeNode::None;
static void check(bool yes,const char* label){
    if(!yes){++fails;std::printf("FAIL %s\n",label);}
}
static bool same(const char* a,const char* b){return a&&b&&!std::strcmp(a,b);}
static uint16_t next(uint16_t i){
    auto* n=doc.get(i);return n?n->next_sibling:NO;
}
static uint16_t child(uint16_t i,const char* name){return doc.first_named_child(i,name);}
static uint16_t first(uint16_t i){auto* n=doc.get(i);return n?n->first_child:NO;}
static bool text(uint16_t i,const char* expected){
    auto* n=doc.get(i);return n&&n->kind==NodeKind::Text&&same(n->text,expected);
}
static bool graph(){
    const size_t count=doc.count();
    if(count>TreeBuilder::kMaxNodes || count<4)return false;
    size_t seen[TreeBuilder::kMaxNodes]{};
    for(size_t i=0;i<count;++i){
        auto* node=doc.get(static_cast<uint16_t>(i));
        if(!node || (i==0 && node->parent!=NO) ||
           (i>0 && (node->parent>=count||node->parent==NO)))return false;
        uint16_t c=node->first_child,last=NO;
        for(size_t j=0;c!=NO && j<=count;++j){
            if(c>=count || doc.get(c)->parent!=i || ++seen[c]!=1)return false;
            last=c;c=doc.get(c)->next_sibling;
        }
        if(c!=NO||last!=node->last_child)return false;
        size_t steps=0;
        for(uint16_t p=static_cast<uint16_t>(i);p!=NO;p=doc.get(p)->parent){
            if(++steps>count)return false;
        }
    }
    for(size_t i=1;i<count;++i)if(seen[i]!=1)return false;
    return true;
}
static void parse(const char* html){
    if(!doc.build(html,std::strlen(html))){
        std::printf("FAIL build code %d (%s)\n",int(doc.error()),html);++fails;
    }
}
int main(){
    {
        parse("<b><i><p>X</b>Y</i>Z</p>");
        uint16_t body=doc.body_index(),b=child(body,"b"),i=next(b);
        uint16_t p=child(i,"p");
        check(b!=NO&&i!=NO&&p!=NO,"intermediate italic and paragraph relocation");
        check(child(p,"b")!=NO&&text(first(child(p,"b")),"X"),
              "adoption clones bold within furthest block");
        check(graph(),"intermediate formatter graph");
    }
    {
        parse("<b><em><i><div>X</div></b>Y</em>Z</i>");
        check(graph(),"multiple intermediates retained without cycles");
        check(child(doc.body_index(),"div")==NO,
              "intermediate elements reparent block, not top-level block");
    }
    {
        parse("<em class='a'><i id='x'><div>inner</em>tail</i></div>");
        check(graph(),"intermediate attribute copies graph");
    }
    {
        parse("<b><i><p>X</b>Y</i>Z</p><strong>K</strong>");
        uint16_t b=child(doc.body_index(),"b");
        check(b!=NO && graph(),"continuation after adoption");
        check(child(doc.body_index(),"strong")!=NO,"subsequent formatting accepted");
    }
    {
        parse("<b>1<p>2</b>3</p>");
        uint16_t b=child(doc.body_index(),"b"),p=next(b);
        check(b!=NO&&text(first(b),"1"),"H3B1 original bold");
        check(p!=NO&&child(p,"b")!=NO&&text(first(child(p,"b")),"2"),
              "H3B1 cloned bold preserved");
        check(text(next(child(p,"b")),"3"),"H3B1 post-close remains plain");
        check(graph(),"direct furthest block invariant");
    }
    {
        parse("<p><b>1<i>2</b>3</i>4");
        uint16_t p=child(doc.body_index(),"p"),b=child(p,"b");
        uint16_t second_i=next(b);
        check(b!=NO&&second_i!=NO&&text(first(second_i),"3"),
              "H3B1 no-furthest reconstruction");
        check(graph(),"nonblock recovery invariant");
    }
    {
        parse("<b><p><i>one</b>two</i></p>");
        check(graph(),"formatting element after furthest block");
    }
    {
        parse("<table><tr><td><em><p>X</em>Y</p></td></tr></table>");
        check(graph(),"adoption inside table cell does not orphan nodes");
    }
    {
        parse("<p><a href='/a'>one<a href='/b'>two</a>three</p>");
        check(graph(),"nested anchors preserved after adoption upgrade");
    }
    {
        const char* chunks[]={
            "<b>","<i>","<em>","<strong>","<a href=x>","<p>","<div>",
            "<table>","<tr>","<td>","</td>","</tr>","</table>",
            "</div>","</p>","</b>","</i>","</em>","</strong>","</a>","X"
        };
        unsigned state=0x49bcd367u;
        for(size_t round=0;round<600;++round){
            std::string input="<body>";
            for(size_t j=0;j<65;++j){
                state=state*1664525u+1013904223u;
                input+=chunks[(state>>16)%(sizeof(chunks)/sizeof(chunks[0]))];
            }
            const bool accepted=doc.build(input.data(),input.size());
            if(accepted)check(graph(),"malformed mixed content graph");
            else check(doc.error()!=BuildError::None,"explicit rejected malformed status");
        }
    }
    if(fails){std::printf("FAIL H3B2 %d checks\n",fails);return 1;}
    std::puts("PASS H3B2 formatting relocation, bookmark and graph stress tests");
    return 0;
}
