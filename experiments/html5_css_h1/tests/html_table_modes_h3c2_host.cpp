#include "trinity_html5_tree.hpp"
#include <cstdio>
#include <cstring>
#include <string>
using namespace trinity::html5;
static TreeBuilder doc;
static constexpr uint16_t NO=TreeNode::None;
static int failures=0;
static void check(bool x,const char* what){if(!x){++failures;std::printf("FAIL %s\n",what);}}
static bool eq(const char* a,const char* b){return a && b && std::strcmp(a,b)==0;}
static uint16_t first(uint16_t n){auto* v=doc.get(n);return v?v->first_child:NO;}
static uint16_t next(uint16_t n){auto* v=doc.get(n);return v?v->next_sibling:NO;}
static uint16_t child(uint16_t n,const char* tag){return doc.first_named_child(n,tag);}
static bool text(uint16_t n,const char* expected){
    auto* v=doc.get(n);return v&&v->kind==NodeKind::Text&&eq(v->text,expected);
}
static bool tag(uint16_t n,const char* expected){
    auto* v=doc.get(n);return v&&v->kind==NodeKind::Element&&eq(v->name,expected);
}
static void parse(const char* input){
    if(!doc.build(input,std::strlen(input))){
        ++failures;std::printf("FAIL parse error=%d src=%.100s\n",int(doc.error()),input);
    }
}
static bool graph(){
    size_t n=doc.count(),seen[TreeBuilder::kMaxNodes]{};
    if(n<4||n>TreeBuilder::kMaxNodes)return false;
    for(size_t i=0;i<n;++i){
        auto* p=doc.get(static_cast<uint16_t>(i));
        if(!p || (i==0&&p->parent!=NO) || (i>0&&(p->parent==NO||p->parent>=n)))
            return false;
        uint16_t last=NO;size_t steps=0;
        for(uint16_t c=p->first_child;c!=NO;c=doc.get(c)->next_sibling){
            if(c>=n||doc.get(c)->parent!=i || ++seen[c]!=1 || ++steps>n)return false;
            last=c;
        }
        if(last!=p->last_child)return false;
        uint16_t a=static_cast<uint16_t>(i);steps=0;
        while(a!=NO){if(++steps>n)return false;a=doc.get(a)->parent;}
    }
    for(size_t i=1;i<n;++i)if(seen[i]!=1)return false;
    return true;
}
int main(){
    {
        parse("<table>  \n A<tr><td>B</td></tr></table>");
        uint16_t foster=first(doc.body_index()),table=next(foster);
        check(text(foster,"  \n A"),"mixed table-text run fosters whitespace and nonwhite together");
        check(tag(table,"table"),"mixed run before table");
        check(text(first(child(child(child(table,"tbody"),"tr"),"td")),"B"),
              "cell contents preserved");
        check(graph(),"mixed run graph");
    }
    {
        parse("<table> \n <tr><td>B</td></tr></table>");
        uint16_t table=first(doc.body_index());
        check(tag(table,"table"),"pure whitespace not fostered");
        check(text(first(table)," \n "),"pure whitespace remains inside table");
        check(graph(),"pure whitespace graph");
    }
    {
        std::string src="<table>";
        src+=std::string(1200,' ');
        src+="Z<tr><td>cell</td></tr></table>";
        bool success=doc.build(src.data(),src.size());
        check(success,"multi-token table text parse");
        if(success){
            size_t chars=0;bool valid=true,found_z=false;
            uint16_t p=first(doc.body_index()),table=NO;
            for(size_t guard=0;p!=NO&&guard<doc.count();++guard,p=next(p)){
                const auto* node=doc.get(p);
                if(node->kind!=NodeKind::Text){table=p;break;}
                chars+=node->text_len;
                for(size_t k=0;k<node->text_len;++k)
                    if(node->text[k]=='Z')found_z=true;
                    else if(node->text[k]!=' ')valid=false;
            }
            check(chars==1201 && found_z && valid,
                  "all 1201 bytes fostered even across tokenizer chunks");
            check(tag(table,"table"),"table after long run");
            check(graph(),"multi-token foster graph");
        }
    }
    {
        parse("<table>hello<!-- comment --><tr><td>X</td></tr></table>");
        uint16_t foster=first(doc.body_index()),table=next(foster);
        check(text(foster,"hello"),"pending run flushed before comment");
        check(doc.get(first(table))->kind==NodeKind::Comment,
              "comment stored inside table");
        check(graph(),"comment flush graph");
    }
    {
        parse("<table><caption><b>Title</b><tr><td>One</td></tr></table>");
        uint16_t table=child(doc.body_index(),"table");
        uint16_t caption=child(table,"caption");
        uint16_t tbody=child(table,"tbody");
        check(tag(caption,"caption") && text(first(child(caption,"b")),"Title"),
              "caption implied closing before row");
        check(tag(tbody,"tbody") && next(caption)==tbody,
              "caption and tbody are sibling table children");
        check(text(first(child(child(tbody,"tr"),"td")),"One"),
              "row following caption");
        check(graph(),"caption implicit close graph");
    }
    {
        parse("<table><caption>A</table>B");
        uint16_t table=child(doc.body_index(),"table");
        check(text(first(child(table,"caption")),"A"),"caption closed by end table");
        check(text(next(table),"B"),"text after table not swallowed by caption");
        check(graph(),"caption closed by table graph");
    }
    {
        parse("<table><col><col><tr><td>A</td></tr></table>");
        uint16_t table=child(doc.body_index(),"table");
        uint16_t group=child(table,"colgroup"),tbody=next(group);
        check(tag(group,"colgroup")&&doc.get(group)->implied,"col implies colgroup");
        check(tag(first(group),"col")&&tag(next(first(group)),"col"),"columns in implied group");
        check(tag(tbody,"tbody"),"row exits colgroup");
        check(graph(),"implied colgroup graph");
    }
    {
        parse("<table><colgroup><col></colgroup><thead><tr><th>A<tbody><tr><td>B</table>");
        uint16_t table=child(doc.body_index(),"table");
        uint16_t group=child(table,"colgroup"),thead=next(group),tbody=next(thead);
        check(tag(group,"colgroup")&&!doc.get(group)->implied,"explicit colgroup");
        check(tag(thead,"thead")&&tag(tbody,"tbody"),"rowgroup transitions");
        check(text(first(child(child(thead,"tr"),"th")),"A"),"thead cell");
        check(text(first(child(child(tbody,"tr"),"td")),"B"),"tbody cell");
        check(graph(),"rowgroup transition graph");
    }
    {
        std::string src="<table>"+std::string(8200,' ')+"X</table>";
        const bool ok=doc.build(src.data(),src.size());
        check(!ok&&doc.error()==BuildError::TokenOverflow,
              "overlarge pending table run fails explicitly");
    }
    {
        static const char* tags[]={
            "<table>","<tbody>","<thead>","<tfoot>","<caption>","<colgroup>",
            "<tr>","<td>","<th>","<col>","<b>","<p>","<i>","</i>",
            "</table>","</tbody>","</caption>","</colgroup>","</tr>",
            "</td>","</th>","text"," \n "
        };
        uint32_t seed=0x7617d561u;
        for(unsigned trial=0;trial<320;++trial){
            std::string s="<body>";
            for(unsigned k=0;k<50;++k){
                seed=seed*1664525u+1013904223u;
                s+=tags[(seed>>16)%(sizeof(tags)/sizeof(tags[0]))];
            }
            bool success=doc.build(s.data(),s.size());
            if(success)check(graph(),"stress ownership graph");
            else check(doc.error()!=BuildError::None,"stress rejection status");
        }
    }
    if(failures){
        std::printf("FAIL H3C2 %d cases\n",failures);return 1;
    }
    std::puts("PASS H3C2 table text buffers, captions, colgroups, rowgroups, stress");
    return 0;
}
