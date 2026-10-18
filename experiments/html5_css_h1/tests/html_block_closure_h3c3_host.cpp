#include "trinity_html5_tree.hpp"
#include <cstdio>
#include <cstring>
using namespace trinity::html5;
static TreeBuilder tree;
static int failures=0;
static constexpr uint16_t NO=TreeNode::None;
static uint16_t next(uint16_t i) {
    const TreeNode* n=tree.get(i);return n?n->next_sibling:NO;
}
static void check(bool value,const char* name) {
    if(!value){++failures;std::printf("FAIL %s\n",name);}
}
static bool eq(const char* a,const char* b){
    return a&&b&&std::strcmp(a,b)==0;
}
int main(){
    static const char* blocks[]={
        "center","dir","fieldset","menu",
        "search","summary","listing","div","article"
    };
    for(const char* block:blocks) {
        char source[128]{};
        std::snprintf(source,sizeof(source),"<p>alpha<%s>beta</%s>",
                      block,block);
        check(tree.build(source,std::strlen(source)),"parse implied p end");
        const uint16_t p=tree.first_named_child(tree.body_index(),"p");
        const uint16_t sibling=next(p);
        const TreeNode* p_node=tree.get(p);
        const TreeNode* sibling_node=tree.get(sibling);
        check(p_node && p_node->first_child!=NO &&
              eq(tree.get(p_node->first_child)->text,"alpha"),
              "first p owns original text");
        check(sibling_node && eq(sibling_node->name,block),
              "block is sibling of implicitly closed paragraph");
        check(sibling_node && sibling_node->first_child!=NO &&
              eq(tree.get(sibling_node->first_child)->text,"beta"),
              "block owns its text");
    }
    {
        const char markup[]="<p>foo<dialog>bar<p>baz";
        check(tree.build(markup,std::strlen(markup)),"dialog reference parse");
        const uint16_t p=tree.first_named_child(tree.body_index(),"p");
        const uint16_t d=tree.first_named_child(p,"dialog");
        check(d!=NO,"dialog remains inside open paragraph");
        const uint16_t second=next(p);
        check(second!=NO && eq(tree.get(second)->name,"p"),
              "second paragraph after implicit p closure");
    }
    if(failures){std::printf("FAIL H3C3 block closure (%d)\n",failures);return 1;}
    std::puts("PASS H3C3 block implied-paragraph closure cases");
    return 0;
}
