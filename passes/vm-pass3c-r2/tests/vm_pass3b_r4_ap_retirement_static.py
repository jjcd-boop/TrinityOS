from pathlib import Path
root=Path(__file__).resolve().parents[1]
sched_h=(root/'include/trinity/scheduler.hpp').read_text()
sched=(root/'src/kernel/scheduler.cpp').read_text()
smp=(root/'src/kernel/smp.cpp').read_text()
checks=[
 ('scheduler API present','retire_current_to_idle(u32 cpu, Thread& current)' in sched_h),
 ('scheduler requires retirement request','retirement_requested.load(std::memory_order_acquire) == 0u' in sched),
 ('scheduler requires current ownership','current_thread(cpu) != &current' in sched and 'current.owner_cpu.load(std::memory_order_acquire) != cpu' in sched),
 ('scheduler terminates detached thread','ThreadState::Terminated' in sched and 'publish_current_thread(cpu, nullptr)' in sched),
 ('fallback only when no replacement','if (!scheduler::pick_interrupt_candidate(cpu, selection))' in smp and 'retire_user_to_ap_idle' in smp),
 ('fallback requires private retiring user','!current.private_user_task' in smp and 'current.retirement_requested.load(std::memory_order_acquire) == 0u' in smp),
 ('fallback leaves user CR3','mmu::kernel_template_cr3()' in smp and 'mmu::switch_address_space(kernel_cr3)' in smp),
 ('fallback clears residency','previous_space->active_cpu_mask.fetch_and(~bit' in smp),
 ('rollback restores user CR3','previous_space->active_cpu_mask.fetch_or(bit' in smp and 'mmu::switch_address_space(previous_space->cr3)' in smp),
 ('kernel idle iret target','frame.cs = 0x08u' in smp and 'trinity_ap_scheduler_idle_resume' in smp),
 ('idle stays scheduler capable','gui::service_background_launch()' in smp and 'cpu_halt_once()' in smp),
]
for name,ok in checks:
    print(('PASS ' if ok else 'FAIL ')+name)
if not all(ok for _,ok in checks): raise SystemExit(1)
print(f'VM Pass 3B-R4 AP retirement static audit {len(checks)}/{len(checks)} PASS')
