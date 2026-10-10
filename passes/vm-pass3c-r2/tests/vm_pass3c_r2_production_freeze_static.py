#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
calc=(ROOT/'src/user/calculator_task.cpp').read_text()
guip=(ROOT/'include/trinity/gui_ipc.hpp').read_text()
guik=(ROOT/'src/kernel/gui_runtime.cpp').read_text()
vmh=(ROOT/'include/trinity/vm.hpp').read_text()
sched=(ROOT/'src/kernel/scheduler.cpp').read_text()
smp=(ROOT/'src/kernel/smp.cpp').read_text()
checks=[]
def ck(name, cond):
    checks.append((name,bool(cond)))
    print(('PASS' if cond else 'FAIL'), name)
ck('qualification startup touch removed', 'HeapProbe' not in calc and 'Pattern=0x5452494e49540000ull' not in calc)
ck('heap layout contract retained', 'PrivateAppHeapBase=0x70000000ull' in guip and 'PrivateAppHeapBytes=256u*1024u' in guip)
ck('all private graphical apps reserve sparse heap', 'loaded==elf::LoadError::Success&&graphical(id)' in guik and 'reserve_anonymous_prelive(*space,heap_begin,heap_end' in guik)
ck('fault-driven anonymous materialization retained', 'vm::FaultClass::NotPresent&&vf.region_type==vm::RegionType::Anonymous' in guik and 'materialize_zero_page' in guik)
ck('production vm telemetry label used', '[vm] lazy page materialized app=' in guik and '[vm3c]' not in guik)
ck('persistent reclaim cursor retained', 'vm::ReclaimCursor vm_reclaim_cursor{};' in guik and 's.vm_reclaim_cursor,&reclaimed' in guik)
ck('reclaim cursor is reset only at lifecycle boundaries', 's.segment=s.va=0;s.vm_reclaim_cursor={};s.input.reset()' in guik and 's.segment=0;s.va=0;s.vm_reclaim_cursor={};s.close.store' in guik)
ck('bounded reclaim retained', 'token,16u,s.vm_reclaim_cursor' in guik and 'if(rr==vm::ReclaimResult::More)return;' in guik)
ck('VM comments describe production first-touch semantics', 'production first-touch primitive' in vmh)
ck('AP retirement fallback retained', 'retire_current_to_idle' in sched and 'retire_user_to_ap_idle' in smp)
failed=[n for n,v in checks if not v]
print(f'VM Pass 3C-R2 production freeze static audit {len(checks)-len(failed)}/{len(checks)} ' + ('PASS' if not failed else 'FAIL'))
raise SystemExit(1 if failed else 0)
