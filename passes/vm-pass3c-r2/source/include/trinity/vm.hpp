#pragma once

#include "base.hpp"
#include "mmu.hpp"
#include <atomic>

namespace trinity::vm {

constexpr usize MaxRegions = 48;
constexpr usize MaxSpaces = 32;

enum PermissionBits : u32 {
    Read    = 1u << 0,
    Write   = 1u << 1,
    Execute = 1u << 2,
    User    = 1u << 3,
    Shared  = 1u << 4,
};

enum class RegionType : u8 {
    Invalid = 0,
    Image,
    Anonymous,
    Stack,
    Guard,
    SharedMemory,
    File,
    Device,
};

// Region policy separates a region's virtual existence from leaf-page residency.
// ELF image/stack regions remain RequirePresent. Anonymous sparse regions may be
// ZeroFill + FaultMaterializable and are populated one page at a time on demand.
enum RegionPolicyBits : u8 {
    RegionPolicyNone             = 0u,
    RegionRequirePresent         = 1u << 0,
    RegionFaultMaterializable    = 1u << 1,
    RegionZeroFill               = 1u << 2,
    RegionOwnsPhysicalFrames     = 1u << 3,
};

enum class MaterializeResult : u8 {
    Success = 0,
    AlreadyPresent,
    NotEligible,
    InvalidState,
    InvalidAddress,
    AuthorityDenied,
    OutOfMemory,
    DirectMapFailed,
    MapFailed,
    MappingCommittedShootdownFailed,
};

enum class ReclaimResult : u8 {
    Complete = 0,
    More,
    InvalidState,
    InvalidArgument,
    QueryFailed,
    UnmapFailed,
    FreeFailed,
};

struct ReclaimCursor {
    u32 region_index{0};
    u32 reserved{0};
    uintptr next_page{0};
};

// Page-fault classification remains separate from the generic exception path.
// The private-app policy layer may resolve an eligible anonymous NotPresent fault;
// guard/protection/out-of-region faults continue through containment.
enum class FaultClass : u8 {
    None = 0,
    NoVmSpace,
    SpaceNotReady,
    OutsideRegion,
    GuardPage,
    NotPresent,
    WriteViolation,
    ExecuteViolation,
    UserViolation,
    ProtectionViolation,
    PagingCorruption,
};

struct FaultInfo {
    FaultClass classification{FaultClass::None};
    RegionType region_type{RegionType::Invalid};
    u16 reserved0{0};
    u32 owner_domain{0};
    u32 region_permissions{0};
    uintptr region_start{0};
    uintptr region_end{0};
};

enum class SpaceState : u8 {
    Empty = 0,
    Building,
    Ready,
    Destroying,
    Dead,
};

struct Region {
    uintptr start{0};
    uintptr end{0};
    u32 permissions{0};
    RegionType type{RegionType::Invalid};
    u8 policy{RegionPolicyNone};
    u16 reserved0{0};
    u32 owner_domain{0};
    u64 backing_id{0};
};
static_assert(sizeof(Region) == 40);

// Sidecar VM metadata. Deliberately not embedded in PreliveImage/ControlBlock:
// Pass 1 must not perturb established boot/runtime structure layouts.
struct VmSpace {
    mmu::AddressSpace* page_tables{nullptr};
    u32 owner_domain{0};
    std::atomic<u8> state{static_cast<u8>(SpaceState::Empty)};
    u8 reserved0[3]{};
    std::atomic<u32> generation{0};
    u32 region_count{0};
    u32 reserved1{0};
    std::atomic<u64> resident_pages{0};
    std::atomic<u64> anonymous_resident_pages{0};
    Region regions[MaxRegions]{};
};

// Claim/lookup a sidecar record for an existing private address space.
VmSpace* acquire(mmu::AddressSpace& page_tables, u32 owner_domain);
VmSpace* lookup(mmu::AddressSpace& page_tables);
const VmSpace* lookup(const mmu::AddressSpace& page_tables);
void abandon(mmu::AddressSpace& page_tables);

bool add_region(VmSpace& space, uintptr start, uintptr end, u32 permissions,
                RegionType type, bool materialized = true, u64 backing_id = 0);
// Reserve an anonymous user range without allocating physical pages.
bool reserve_anonymous(VmSpace& space, uintptr start, uintptr end, u32 permissions,
                       u64 backing_id = 0);
// Extend a READY but never-executed private address space without changing the
// ELF-loader ABI. This is legal only while active_cpu_mask is zero; the caller
// must re-run audit_ready_mappings() before scheduler admission.
bool reserve_anonymous_prelive(VmSpace& space, uintptr start, uintptr end, u32 permissions,
                               u64 backing_id = 0);
bool publish_ready(VmSpace& space);
bool audit_ready_mappings(const VmSpace& space);
FaultInfo classify_page_fault(const VmSpace& space, uintptr address, u64 error);
FaultInfo classify_page_fault(const mmu::AddressSpace& page_tables, uintptr address, u64 error);
// Bounded single-page zero-fill materialization. The caller supplies the same
// capability token already authorized to edit this private address space. This
// is the production first-touch primitive for eligible anonymous regions.
MaterializeResult materialize_zero_page(VmSpace& space, uintptr address, u64 fault_error,
                                        u32 allocation_cpu, const mmu::CapabilityToken& token,
                                        uintptr* physical_out = nullptr);
// Bounded teardown sweep for pages created by reserve_anonymous/materialize_zero_page.
// Existing ELF/image/stack pages remain owned by their established teardown path;
// callers persist ReclaimCursor across maintenance slices until Complete.
ReclaimResult reclaim_anonymous_pages(VmSpace& space, u32 allocation_cpu,
                                      const mmu::CapabilityToken& token, u32 page_budget,
                                      ReclaimCursor& cursor, u32* reclaimed_pages = nullptr);
const Region* find_region(const VmSpace& space, uintptr address);
bool range_owned(const VmSpace& space, uintptr start, usize bytes, u32 required_permissions = 0);
bool begin_destroy(mmu::AddressSpace& page_tables);
void finish_destroy(mmu::AddressSpace& page_tables);
SpaceState state(const VmSpace& space);
const char* state_name(SpaceState state);
const char* region_type_name(RegionType type);
const char* fault_class_name(FaultClass classification);

} // namespace trinity::vm
