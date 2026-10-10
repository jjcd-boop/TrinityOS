#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
guih=(ROOT/'include/trinity/gui_ipc.hpp').read_text()
guir=(ROOT/'src/kernel/gui_runtime.cpp').read_text()
vm=(ROOT/'src/kernel/vm.cpp').read_text()
calc=(ROOT/'src/user/calculator_task.cpp').read_text()
checks=[]
def ck(name,cond):
    checks.append((name,bool(cond)))
    print(('PASS' if cond else 'FAIL'), name)
ck('shared heap base contract', 'PrivateAppHeapBase=0x70000000ull' in guih)
ck('heap reservation is 256 KiB virtual', 'PrivateAppHeapBytes=256u*1024u' in guih)
ck('all graphical private apps reserve heap', 'loaded==elf::LoadError::Success&&graphical(id)' in guir)
ck('heap reservation remains post-loader/pre-admission', 'reserve_anonymous_prelive(*space,heap_begin,heap_end' in guir)
ck('heap reservation re-audits page tables', '!vm::audit_ready_mappings(*space)' in guir)
ck('heap backing identity includes app id', 'HeapBackingTag|static_cast<u64>(id)' in guir)
ck('fault service is anonymous-region generic', 'vf.classification==vm::FaultClass::NotPresent&&vf.region_type==vm::RegionType::Anonymous' in guir)
ck('calculator-only demand gate removed', 's->id==static_cast<u32>(process::Id::Calculator)&&vf.classification' not in guir)
ck('instruction fetch cannot instantiate heap', '(fault_error & (1ull << 4)) != 0u' in vm)
ck('write permission revalidated before materialize', 'write_fault && (region->permissions & Write) == 0u' in vm)
ck('read permission revalidated before materialize', '!write_fault && (region->permissions & Read) == 0u' in vm)
ck('qualification-only startup heap probe removed', 'HeapProbe' not in calc and 'VM Pass 3C qualification probe' not in calc)
ck('lazy allocation remains fault-driven', '[vm] lazy page materialized app=' in guir)
ck('heap reservation remains sparse at admission', 'reserve_anonymous_prelive(*space,heap_begin,heap_end' in guir)
ck('R4 AP retirement fallback remains present', 'retire_current_to_idle' in (ROOT/'src/kernel/scheduler.cpp').read_text())
failed=[n for n,v in checks if not v]
print(f'VM Pass 3C/R2 production lazy heap static audit {len(checks)-len(failed)}/{len(checks)} ' + ('PASS' if not failed else 'FAIL'))
raise SystemExit(1 if failed else 0)
