#include "trinity_html5_tree.hpp"
#include <cstdio>
#include <cstring>
using namespace trinity::html5;
static TreeBuilder document;
static void print(uint16_t parent,size_t depth) {
    const auto* p=document.get(parent);
    if(!p || depth>TreeBuilder::kMaxDepth)return;
    for(uint16_t i=p->first_child;i!=TreeNode::None;){
        const TreeNode* n=document.get(i);
        if(!n)return;
        if(n->kind==NodeKind::Element){
            std::printf("E:%s\n",n->name);
            print(i,depth+1);
            std::puts("/E");
        }else if(n->kind==NodeKind::Text){
            std::fputs("T:",stdout);
            for(size_t k=0;k<n->text_len;++k){
                const char ch=n->text[k];
                if(ch=='\n')std::fputs("\\n",stdout);
                else if(ch=='\r')std::fputs("\\r",stdout);
                else if(ch=='\t')std::fputs("\\t",stdout);
                else if(ch=='\\')std::fputs("\\\\",stdout);
                else std::putchar(static_cast<unsigned char>(ch));
            }
            std::putchar('\n');
        }
        i=n->next_sibling;
    }
}
int main(int argc,char** argv){
    if(argc!=2)return 3;
    const char* html=argv[1];
    if(!document.build(html,std::strlen(html)))return 2;
    print(document.body_index(),0);
    return 0;
}
