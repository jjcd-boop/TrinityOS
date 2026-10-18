#include "trinity_html5_segmented_nodes.hpp"
#include <new>

namespace trinity::html5 {

size_t SegmentedNodeArena::segment_bytes() noexcept {return sizeof(Segment);}
size_t SegmentedNodeArena::segment_alignment() noexcept {return alignof(Segment);}

SegmentedNodeArena::SegmentedNodeArena(
        NodeArenaMemory memory,size_t nodes,size_t bytes) noexcept
    : memory_(memory), max_nodes_(nodes), max_bytes_(bytes) {
    if(!memory_.allocate || !memory_.release || max_nodes_==0 ||
       max_nodes_>kMaxNodes || max_nodes_>=TreeNode::None ||
       max_bytes_<sizeof(Segment))error_=NodeArenaError::BadConfiguration;
}
SegmentedNodeArena::~SegmentedNodeArena() noexcept {clear();}
bool SegmentedNodeArena::add_segment() noexcept {
    if(error_!=NodeArenaError::None)return false;
    if(segment_count_>=kMaxSegments){
        error_=NodeArenaError::NodeBudget;return false;
    }
    if(reserved_bytes_>max_bytes_ ||
       max_bytes_-reserved_bytes_<sizeof(Segment)){
        error_=NodeArenaError::ByteBudget;return false;
    }
    void* raw=memory_.allocate(sizeof(Segment),alignof(Segment),memory_.context);
    if(!raw){error_=NodeArenaError::AllocatorFailure;return false;}
    // Callback contract includes alignment; fail closed if a provider
    // violates it, and leave the existing arena unchanged.
    if((reinterpret_cast<uintptr_t>(raw)%alignof(Segment))!=0){
        memory_.release(raw,sizeof(Segment),alignof(Segment),memory_.context);
        error_=NodeArenaError::AllocatorFailure;return false;
    }
    Segment* segment=::new(raw)Segment{};
    segments_[segment_count_++]=segment;
    reserved_bytes_+=sizeof(Segment);
    return true;
}
bool SegmentedNodeArena::append(const TreeNode& initial,uint16_t& handle) noexcept {
    handle=TreeNode::None;
    if(error_!=NodeArenaError::None)return false;
    if(count_>=max_nodes_ || count_>=kMaxNodes){
        error_=NodeArenaError::NodeBudget;return false;
    }
    if(count_==capacity() && !add_segment())return false;
    const size_t page=count_/kNodesPerSegment;
    const size_t offset=count_%kNodesPerSegment;
    segments_[page]->nodes[offset]=initial;
    handle=static_cast<uint16_t>(count_++);
    return true;
}
TreeNode* SegmentedNodeArena::get(uint16_t index) noexcept {
    if(index==TreeNode::None || index>=count_)return nullptr;
    return &segments_[index/kNodesPerSegment]->nodes[index%kNodesPerSegment];
}
const TreeNode* SegmentedNodeArena::get(uint16_t index) const noexcept {
    if(index==TreeNode::None || index>=count_)return nullptr;
    return &segments_[index/kNodesPerSegment]->nodes[index%kNodesPerSegment];
}
void SegmentedNodeArena::clear() noexcept {
    for(size_t i=0;i<segment_count_;++i){
        segments_[i]->~Segment();
        memory_.release(segments_[i],sizeof(Segment),alignof(Segment),memory_.context);
        segments_[i]=nullptr;
    }
    count_=segment_count_=reserved_bytes_=0;
    error_=(!memory_.allocate || !memory_.release || max_nodes_==0 ||
            max_nodes_>kMaxNodes || max_nodes_>=TreeNode::None ||
            max_bytes_<sizeof(Segment)) ?
            NodeArenaError::BadConfiguration:NodeArenaError::None;
}
}  // namespace trinity::html5
