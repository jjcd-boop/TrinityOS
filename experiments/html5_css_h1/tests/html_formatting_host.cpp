#include "trinity_html5_tree.hpp"
#include <cstdio>
#include <cstring>
#include <string>
using namespace trinity::html5;

static TreeBuilder doc;
static int failures=0;
static constexpr uint16_t NO=TreeNode::None;
static void check(bool v,const char* label){
    if(!v){++failures;std::printf("FAIL %s\n",label);}
}
static bool eq(const char* a,const char* b){return a&&b&&std::strcmp(a,b)==0;}
static uint16_t first(uint16_t node){const TreeNode* n=doc.get(node);return n?n->first_child:NO;}
static uint16_t next(uint16_t node){const TreeNode* n=doc.get(node);return n?n->next_sibling:NO;}
static uint16_t child(uint16_t node,const char* name){return doc.first_named_child(node,name);}
static bool text(uint16_t node,const char* value){
    const TreeNode* n=doc.get(node);
    return n&&n->kind==NodeKind::Text&&eq(n->text,value);
}
static bool named(uint16_t node,const char* name){
    const TreeNode* n=doc.get(node);
    return n&&n->kind==NodeKind::Element&&eq(n->name,name);
}
static void parse(const char* source){
    if(!doc.build(source,std::strlen(source))){
        ++failures;
        std::printf("FAIL parse status=%d source=%s\n",int(doc.error()),source);
    }
}
// Check exact graph ownership, child-list termination and acyclic ancestry.
static bool valid_graph(){
    const size_t n=doc.count();
    if(n<4 || n>TreeBuilder::kMaxNodes)return false;
    size_t occurrences[TreeBuilder::kMaxNodes]{};
    for(size_t i=0;i<n;++i){
        const TreeNode* p=doc.get(static_cast<uint16_t>(i));
        if(!p)return false;
        if(i==0 && p->parent!=NO)return false;
        if(i>0 && (p->parent==NO||p->parent>=n))return false;
        uint16_t last=NO;
        uint16_t cur=p->first_child;
        size_t guard=0;
        while(cur!=NO){
            if(cur>=n || ++guard>n)return false;
            const TreeNode* c=doc.get(cur);
            if(c->parent!=i)return false;
            if(++occurrences[cur]>1)return false;
            last=cur;
            cur=c->next_sibling;
        }
        if(last!=p->last_child)return false;
        uint16_t ancestor=static_cast<uint16_t>(i);
        for(size_t depth=0;ancestor!=NO;++depth){
            if(depth>n)return false;
            ancestor=doc.get(ancestor)->parent;
        }
    }
    for(size_t i=1;i<n;++i)if(occurrences[i]!=1)return false;
    return true;
}
int main(){
    {
        parse("<p><b>1<i>2</b>3</i>4");
        uint16_t p=child(doc.body_index(),"p");
        uint16_t b=child(p,"b"),firsti=child(b,"i");
        uint16_t secondi=next(b),tail=next(secondi);
        check(b!=NO&&text(first(b),"1")&&named(firsti,"i")&&
              text(first(firsti),"2"),"adoption no block first segment");
        check(named(secondi,"i")&&text(first(secondi),"3"),
              "reconstructed italic after misnested b");
        check(text(tail,"4"),"plain tail after i closure");
        check(valid_graph(),"no block adoption graph");
    }
    {
        parse("<b><i>1</b>2</i>");
        uint16_t b=child(doc.body_index(),"b");
        uint16_t i=child(b,"i"),after=next(b);
        check(b!=NO&&named(i,"i")&&text(first(i),"1"),
              "first misnested emphasis");
        check(named(after,"i")&&text(first(after),"2"),
              "reconstructed emphasis outside closed bold");
        check(valid_graph(),"misnested formatting graph");
    }
    {
        parse("<b>1<p>2</b>3</p>");
        uint16_t b=child(doc.body_index(),"b");
        uint16_t p=next(b);
        uint16_t nested=child(p,"b"),tail=next(nested);
        check(b!=NO&&text(first(b),"1"),"furthest block before formatting");
        check(named(p,"p")&&named(nested,"b")&&text(first(nested),"2"),
              "direct furthest-block relocated and cloned");
        check(text(tail,"3"),"furthest-block continuation is unformatted");
        check(valid_graph(),"furthest block graph invariant");
    }
    {
        parse("<p><a href='/a'>A<a href='/b'>B</a>C</p>");
        uint16_t p=child(doc.body_index(),"p");
        uint16_t a=child(p,"a"),b=next(a),tail=next(b);
        check(named(a,"a")&&text(first(a),"A"),"first hyperlink");
        check(named(b,"a")&&text(first(b),"B"),"nested hyperlink repaired");
        check(text(tail,"C"),"text after repaired hyperlink");
        check(doc.get(a)->attr_count==1&&eq(doc.get(a)->attrs[0].value,"/a"),
              "first href retained");
        check(doc.get(b)->attr_count==1&&eq(doc.get(b)->attrs[0].value,"/b"),
              "second href retained");
        check(valid_graph(),"link repair graph");
    }
    {
        parse("<div><b class='x'>inside</div>outside");
        uint16_t div=child(doc.body_index(),"div");
        uint16_t b=child(div,"b");
        uint16_t restored=next(div);
        check(named(b,"b")&&text(first(b),"inside"),"original bold content");
        check(named(restored,"b")&&text(first(restored),"outside"),
              "formatting reconstructs beyond closed div");
        check(doc.get(restored)->attr_count==1&&
              eq(doc.get(restored)->attrs[0].value,"x"),
              "reconstruction copies formatting attributes");
        check(valid_graph(),"formatting reconstruction graph");
    }
    {
        parse("<table><tr><td><b>one</td><td>two</td></tr></table>");
        uint16_t table=child(doc.body_index(),"table");
        uint16_t row=child(child(table,"tbody"),"tr");
        uint16_t firstcell=child(row,"td"),secondcell=next(firstcell);
        uint16_t b=child(firstcell,"b");
        check(named(b,"b")&&text(first(b),"one"),"cell bold");
        check(named(secondcell,"td")&&text(first(secondcell),"two"),
              "formatting marker prevents leak into next cell");
        check(valid_graph(),"cell marker graph");
    }
    {
        parse("<table><caption><b>head</caption><tr><td>body</td></tr></table>");
        uint16_t table=child(doc.body_index(),"table");
        uint16_t caption=child(table,"caption");
        uint16_t td=child(child(table,"tbody"),"tr");
        check(named(caption,"caption")&&text(first(child(caption,"b")),"head"),
              "caption formatting preserved");
        check(text(first(child(td,"td")),"body"),"caption marker cleared");
        check(valid_graph(),"caption marker graph");
    }
    {
        parse("<b><b><b><b>x</b></b></b></b>");
        uint16_t b=child(doc.body_index(),"b");
        uint16_t current=b;
        for(int i=1;i<4 && current!=NO;++i)current=child(current,"b");
        check(current!=NO&&text(first(current),"x"),
              "more than three DOM bold elements still allowed");
        check(valid_graph(),"Noah Ark active list graph");
    }
    {
        parse("<div><b>one</b><em>two</em><strong>three</strong></div>");
        uint16_t div=child(doc.body_index(),"div");
        uint16_t b=child(div,"b"),em=next(b),strong=next(em);
        check(named(b,"b")&&text(first(b),"one"),"ordinary bold");
        check(named(em,"em")&&text(first(em),"two"),"ordinary em");
        check(named(strong,"strong")&&text(first(strong),"three"),"ordinary strong");
        check(valid_graph(),"ordinary formatting regression");
    }
    {
        parse("<p><i>this <b>is</b> text</i></p>");
        uint16_t p=child(doc.body_index(),"p");
        uint16_t i=child(p,"i");
        uint16_t b=child(i,"b");
        check(named(i,"i")&&text(first(i),"this ")&&
              named(b,"b")&&text(first(b),"is")&&
              text(next(b)," text"),"valid formatting nesting");
        check(valid_graph(),"valid formatting graph");
    }
    {
        parse("<table><tr><td><b>outer<table><tr><td>inner</td></tr></table>tail</b></td></tr></table>");
        check(valid_graph(),"nested table with live formatting graph");
    }
    {
        // Malformed deterministic stress with nested formatting, table nodes
        // and block boundaries. Any admitted tree must have coherent links.
        const char* chunks[]={
            "<b>","<i>","<em>","<a href=x>","<p>","<div>","<table>",
            "<tr>","<td>","</td>","</tr>","</table>","</div>","</p>",
            "</b>","</i>","</em>","</a>","x"," y","<br>"
        };
        uint32_t state=0x437ff313u;
        for(size_t trial=0;trial<400;++trial){
            std::string src="<body>";
            for(size_t k=0;k<54;++k){
                state=state*1664525u+1013904223u;
                src+=chunks[(state>>16)%(sizeof(chunks)/sizeof(chunks[0]))];
            }
            const bool success=doc.build(src.c_str(),src.size());
            if(success)check(valid_graph(),"randomized malformed formatting graph");
            else check(doc.error()!=BuildError::None,"reject with explicit error");
        }
    }
    if(failures){
        std::printf("FAIL H3B1 formatting regression (%d failures)\n",failures);
        return 1;
    }
    std::puts("PASS H3B1 active formatting reconstruction and bounded adoption cases");
    return 0;
}
