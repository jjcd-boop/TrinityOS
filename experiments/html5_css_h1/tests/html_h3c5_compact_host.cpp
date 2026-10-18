#include "trinity_html5_segmented_nodes.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cstddef>
using namespace trinity::html5;
#ifdef TRINITY_HTML5_SEGMENTED_ONLY
static_assert(sizeof(TreeBuilder)<32*1024,
              "segmented-only parser must not embed legacy 384-node array");
#else
static_assert(sizeof(TreeBuilder)>64*1024,
              "legacy configuration should retain the compatibility arena");
#endif
static int failures=0;
static void check(bool yes,const char* what){
    if(!yes){++failures;std::printf("FAIL %s\n",what);}
}
struct Stats { size_t live{0}; };
static void* alloc(size_t n,size_t a,void* ctx) noexcept {
    if(a>alignof(std::max_align_t))return nullptr;
    auto* stats=static_cast<Stats*>(ctx);
    void* p=std::malloc(n);
    if(p)stats->live+=n;
    return p;
}
static void release(void* p,size_t n,size_t a,void* ctx) noexcept {
    (void)a;
    if(p)static_cast<Stats*>(ctx)->live-=n;
    std::free(p);
}
int main(){
    static Stats stats{};
    const size_t segment=SegmentedNodeArena::segment_bytes();
    static SegmentedNodeArena arena(NodeArenaMemory{&stats,alloc,release},
                                    6000,segment*750);
    static TreeBuilder tree(arena);
    std::string html="<!doctype html><html><body>";
    for(int i=0;i<1150;++i)html+="<div>A</div>";
    html+="</body></html>";
    check(tree.build(html.data(),html.size()),"segmented-only full document");
    check(tree.count()==2305,"2305 nodes in live parser");
    check(arena.count()==tree.count(),"storage counts agree");
    check(tree.body_index()!=TreeNode::None,"body index assigned");
    check(tree.get(2304)!=nullptr,"last allocated node available");
    check(tree.get(2305)==nullptr,"out-of-range handle rejected");
    check(tree.build("<p>second</p>",13),"rebuild document");
    check(tree.count()==6,"new document has correct skeleton and nodes");
    check(stats.live==arena.reserved_bytes(),"no pages leaked on rebuild");
    arena.clear();check(stats.live==0,"all storage released");
    if(failures){std::printf("FAIL H3C5 %d cases\n",failures);return 1;}
    std::printf("PASS H3C5 segmented-only live DOM, sizeof(TreeBuilder)=%zu bytes\n",sizeof(TreeBuilder));
    return 0;
}
