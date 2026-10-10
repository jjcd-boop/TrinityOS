#pragma once
#include "base.hpp"
namespace trinity::gui {
constexpr u32 Version=6,Capacity=64,Calculator=2,Solum=12,Draw=13,Forge=14,LastGraphical=Forge,Idle=15,Worker2=16,SlotCount=17;
constexpr u32 MaxDamageRects=8;
// Production VM layout contract: each private graphical process receives this
// heap-style anonymous reservation in its own CR3. The range is metadata-only
// at launch; physical pages are zero-filled and materialized one page at a time
// only after a valid Ring-3 first access. Keeping the layout in the shared GUI
// contract prevents kernel/userland address drift without changing structure ABI.
constexpr uintptr PrivateAppHeapBase=0x70000000ull;
constexpr usize PrivateAppHeapBytes=256u*1024u;
// BOOT-AUTO R3 marks Desktop-owned automatic AP admission requests without
// changing the administrator MulticoreControl ABI used by MC2START..MC7START.
constexpr u64 BootAutoAdmissionGeneration=0x5452494E49545933ull; // "TRINITY3"
constexpr bool graphical(u32 id){return id>=1&&id<=LastGraphical&&id!=8;}
constexpr u64 Busy=~0ull;
enum class Operation:u32 {AppStart,AppReady,AppExit,AppCrashed,WindowCreate,WindowDestroy,WindowShow,WindowHide,WindowMove,WindowResize,WindowInvalidate,FocusGained,FocusLost,InputKey,InputMouse,InputScroll,SurfaceCreate,SurfaceReady,SurfaceRelease,ClipboardRead,ClipboardWrite,FileOpen,FileSave,Poll,Compose,Service,Status,RegisterExit,Job,SuiteRead,SuiteWrite,ShellPower,Diagnostics,MulticoreControl};
enum class Result:u32 {Ok,Invalid,Denied,Busy,Full,Stale,NoMemory,NotReady,Unsupported,Failed};
enum class State:u32 {Empty,Starting,Running,Closing,Retiring,Reclaiming,Failed};
enum class Error:u32 {None,Load,Surface,Xstate,Context,Admission,Unmap,Free,Root,CloseTimeout,Fault,LaunchTimeout,DeviceStop,AdmissionTimeout,ReadyTimeout,FirstFrameTimeout,ResourceLimit,Unresponsive};
enum class LaunchPhase:u32 {None,Queued,Admitting,Admitted,Ready,FirstFrame,Failed};
struct Event {u64 generation=0,sequence=0;u32 key=0,buttons=0;i32 x=0,y=0,scroll=0;};
struct DamageRect { i32 x=0,y=0,w=0,h=0; };
struct Message {u32 version=Version;Operation operation{};u32 app=Calculator;Result result=Result::Invalid;u64 generation=0,job=0,payload=0;i32 x=0,y=0,w=0,h=0;Event event{};State state=State::Empty;Error error=Error::None;u32 cpu=~0u,depth=0,close=0,ready=0,thread_state=0,requested_app=0,power_request=0;
 LaunchPhase launch_phase=LaunchPhase::None;u32 launch_attempts=0;u64 launch_elapsed_ms=0;
 char path[128]{};u64 frames=0,backpressure=0,fault_vector=0,fault_rip=0;
 // Browser audit: explicit contained-fault telemetry. Kept on the point Status
 // message instead of expanding the one-page multicore DiagnosticSnapshot.
 u64 fault_rsp=0,fault_address=0,stack_guard=0,last_network_request=0;
 u64 stack_low=0,stack_high=0,last_worker_job=0;
 u32 fault_error=0,guard_hit=0,watchdog_trips=0,crashed=0;
 // R26: Compose returns each retained dirty region independently so Desktop
 // never has to reconstruct a single bounding box around unrelated updates.
 u32 damage_count=0;DamageRect damage[MaxDamageRects]{};};
}
