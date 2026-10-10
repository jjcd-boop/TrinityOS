#include "trinity/app_host.hpp"
#include "trinity/gui_ipc.hpp"
#include "trinity/user_jobs.hpp"
namespace trinity::userland {
void calculator_task_initialize(u64);
void calculator_task_prepare();
bool calculator_task_request(gui::Message&);
[[noreturn]] void calculator_task_exit();
[[noreturn]] void run_calculator_task(u64 id){
 gui::Message entry{};entry.operation=gui::Operation::RegisterExit;entry.payload=reinterpret_cast<u64>(&calculator_task_exit);
 if(!calculator_task_request(entry))calculator_task_exit();
 calculator_task_initialize(id);
 if(id==gui::Idle||id==gui::Worker2){gui::Message worker_ready{};worker_ready.operation=gui::Operation::AppReady;bool worker_ready_ok=false;for(u32 retry=0;retry<64&&!worker_ready_ok;++retry){worker_ready_ok=calculator_task_request(worker_ready);if(!worker_ready_ok)__asm__ volatile("pause":::"memory");}if(!worker_ready_ok)calculator_task_exit();run_worker_tasks();}
 calculator_task_prepare();Cursor cursor{0,0,0,0,true};gui::Message ready{};ready.operation=gui::Operation::AppReady;bool ready_ok=false;for(u32 retry=0;retry<64&&!ready_ok;++retry){ready_ok=calculator_task_request(ready);if(!ready_ok)__asm__ volatile("pause":::"memory");}if(!ready_ok)calculator_task_exit();
 gui::Message launch{};launch.operation=gui::Operation::Poll;if(calculator_task_request(launch)&&id==3&&launch.path[0])explorer_set_start_path(launch.path);
 const auto next=id==2?calculator_loop(cursor):dispatch_app_loop(id,cursor);gui::Message exit{};exit.operation=gui::Operation::AppExit;exit.payload=process_id_for_mode(next);host_copy_launch_path(exit.path,sizeof(exit.path));for(u32 retry=0;retry<64&&!calculator_task_request(exit);++retry)__asm__ volatile("pause":::"memory");calculator_task_exit();
}
}
