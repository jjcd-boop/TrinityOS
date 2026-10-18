#pragma once
#include "trinity_html5_tree.hpp"
#include <stddef.h>
#include <stdint.h>

namespace trinity::html5 {

// Caller-owned page allocation. The service must implement these using its
// bounded Ring-3 allocator; allocate must honor the supplied byte alignment.
struct NodeArenaMemory {
    void* context{nullptr};
    void* (*allocate)(size_t bytes,size_t alignment,void* context) noexcept{nullptr};
    void (*release)(void* pointer,size_t bytes,size_t alignment,void* context) noexcept{nullptr};
};

enum class NodeArenaError : uint8_t {
    None, BadConfiguration, NodeBudget, ByteBudget, AllocatorFailure
};

// Standalone staged design for replacing the 384-node TreeBuilder fixed array.
// NOT YET wired into TreeBuilder. Nodes have stable, 16-bit, non-reused handles.
// A segmented arena isolates allocation failure from the parser logic.
class SegmentedNodeArena {
public:
    static constexpr size_t kNodesPerSegment=8;
    static constexpr size_t kMaxSegments=1024;
    static constexpr size_t kMaxNodes=kNodesPerSegment*kMaxSegments;

    explicit SegmentedNodeArena(NodeArenaMemory memory,
                                size_t max_nodes,size_t max_bytes) noexcept;
    ~SegmentedNodeArena() noexcept;
    SegmentedNodeArena(const SegmentedNodeArena&)=delete;
    SegmentedNodeArena& operator=(const SegmentedNodeArena&)=delete;

    bool append(const TreeNode& initial,uint16_t& handle) noexcept;
    TreeNode* get(uint16_t handle) noexcept;
    const TreeNode* get(uint16_t handle) const noexcept;
    void clear() noexcept;
    size_t count() const noexcept {return count_;}
    size_t capacity() const noexcept {return segment_count_*kNodesPerSegment;}
    size_t reserved_bytes() const noexcept {return reserved_bytes_;}
    size_t node_budget() const noexcept {return max_nodes_;}
    size_t byte_budget() const noexcept {return max_bytes_;}
    NodeArenaError error() const noexcept {return error_;}
    static size_t segment_bytes() noexcept;
    static size_t segment_alignment() noexcept;

private:
    struct Segment {TreeNode nodes[kNodesPerSegment]{};};
    Segment* segments_[kMaxSegments]{};
    NodeArenaMemory memory_{};
    size_t count_{0},segment_count_{0},reserved_bytes_{0};
    size_t max_nodes_{0},max_bytes_{0};
    NodeArenaError error_{NodeArenaError::None};
    bool add_segment() noexcept;
};
}  // namespace trinity::html5
