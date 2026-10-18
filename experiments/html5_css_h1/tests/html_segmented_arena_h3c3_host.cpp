#include "trinity_html5_segmented_nodes.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
using namespace trinity::html5;

struct BudgetAllocator {
    size_t allocations{0},releases{0},live_bytes{0},peak_bytes{0};
    size_t fail_after{SIZE_MAX};
};
static void* allocate(size_t bytes,size_t alignment,void* context) noexcept {
    auto* s=static_cast<BudgetAllocator*>(context);
    if(!s || alignment>alignof(std::max_align_t) ||
       s->allocations>=s->fail_after)return nullptr;
    void* ptr=std::malloc(bytes);
    if(ptr) {
        ++s->allocations;
        s->live_bytes+=bytes;
        if(s->live_bytes>s->peak_bytes)s->peak_bytes=s->live_bytes;
    }
    return ptr;
}
static void release(void* ptr,size_t bytes,size_t alignment,void* context) noexcept {
    (void)alignment;
    auto* s=static_cast<BudgetAllocator*>(context);
    if(s && ptr){
        ++s->releases;s->live_bytes-=bytes;
    }
    std::free(ptr);
}
static NodeArenaMemory adapter(BudgetAllocator& allocator){
    return NodeArenaMemory{&allocator,allocate,release};
}
static int failures=0;
static void check(bool yes,const char* label){
    if(!yes){++failures;std::printf("FAIL %s\n",label);}
}
int main() {
    const auto page=SegmentedNodeArena::segment_bytes();
    check(page>sizeof(TreeNode),"segment metadata size");
    {
        BudgetAllocator alloc{};
        SegmentedNodeArena arena(adapter(alloc),4096,page*512);
        check(arena.error()==NodeArenaError::None,"valid arena configuration");
        TreeNode original{};
        uint16_t first=TreeNode::None, handle=TreeNode::None;
        for(size_t i=0;i<2100;++i){
            TreeNode n{};n.text[0]=static_cast<char>('A'+(i%26));
            if(!arena.append(n,handle)){check(false,"append >384 nodes");break;}
            if(i==0)first=handle;
            check(handle==i,"handles monotonically increasing");
        }
        check(arena.count()==2100,"2100 nodes supported");
        check(arena.capacity()>=2100 && arena.reserved_bytes()<=page*512,
              "arena respects reservation quota");
        check(arena.get(first)!=nullptr && arena.get(first)->text[0]=='A',
              "first stable handle");
        check(arena.get(2099)!=nullptr &&
              arena.get(2099)->text[0]==static_cast<char>('A'+(2099%26)),
              "last node accessible");
        check(arena.get(2100)==nullptr && arena.get(TreeNode::None)==nullptr,
              "reject invalid handles");
        const size_t pages=alloc.allocations;
        arena.clear();
        check(arena.count()==0 && arena.reserved_bytes()==0 &&
              alloc.releases==pages && alloc.live_bytes==0,
              "all pages released on clear");
        check(arena.append(original,handle) && handle==0,"reuse after clear");
    }
    {
        BudgetAllocator alloc{};
        SegmentedNodeArena arena(adapter(alloc),100,page);
        uint16_t h{};
        for(size_t i=0;i<SegmentedNodeArena::kNodesPerSegment;++i)
            check(arena.append(TreeNode{},h),"fill first segment");
        check(!arena.append(TreeNode{},h) &&
              arena.error()==NodeArenaError::ByteBudget &&
              arena.count()==SegmentedNodeArena::kNodesPerSegment,
              "fail closed at exact byte quota");
        check(alloc.live_bytes==page,"failure preserves existing segment");
        arena.clear();
        check(alloc.live_bytes==0,"byte quota cleanup");
    }
    {
        BudgetAllocator alloc{};
        SegmentedNodeArena arena(adapter(alloc),9,page*2);
        uint16_t h{};
        for(size_t i=0;i<9;++i)check(arena.append(TreeNode{},h),"node limit fill");
        check(!arena.append(TreeNode{},h) &&
              arena.error()==NodeArenaError::NodeBudget &&
              arena.count()==9,"node quota reached exactly");
    }
    {
        BudgetAllocator alloc{};
        alloc.fail_after=1;
        SegmentedNodeArena arena(adapter(alloc),100,page*8);
        uint16_t h{};
        for(size_t i=0;i<8;++i)check(arena.append(TreeNode{},h),"first segment");
        check(!arena.append(TreeNode{},h) &&
              arena.error()==NodeArenaError::AllocatorFailure,
              "fail closed when Ring-3 allocator reports OOM");
        check(arena.count()==8 && arena.get(0)!=nullptr &&
              alloc.live_bytes==page,"existing nodes survive allocation failure");
        arena.clear();
        check(alloc.live_bytes==0,"oom cleanup releases allocations");
    }
    {
        BudgetAllocator alloc{};
        SegmentedNodeArena bad(adapter(alloc),0,page);
        check(bad.error()==NodeArenaError::BadConfiguration,
              "zero node budget prohibited");
        uint16_t h{};
        check(!bad.append(TreeNode{},h),"invalid configuration blocks writes");
    }
    if(failures){std::printf("FAIL H3C3 segmented arena (%d)\n",failures);return 1;}
    std::puts("PASS H3C3 2100-node segmented arena, budget and OOM tests");
    return 0;
}
