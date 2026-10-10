from pathlib import Path
root = Path(__file__).resolve().parents[1]
s = (root/'src/kernel/gui_runtime.cpp').read_text()
checks = [
    ('slot owns persistent reclaim cursor', 'vm::ReclaimCursor vm_reclaim_cursor{};' in s),
    ('reclaim passes slot cursor', 's.vm_reclaim_cursor,&reclaimed' in s),
    ('no stack-local anonymous reclaim cursor', 'vm::ReclaimCursor cursor{}' not in s),
    ('launch resets reclaim cursor', 's.segment=s.va=0;s.vm_reclaim_cursor={};s.input.reset()' in s),
    ('successful teardown resets reclaim cursor', 's.segment=0;s.va=0;s.vm_reclaim_cursor={};s.close.store' in s),
    ('anonymous reclaim remains bounded', 'token,16u,s.vm_reclaim_cursor' in s),
    ('More remains deferred not failed', 'if(rr==vm::ReclaimResult::More)return;' in s),
]
failed = 0
for name, ok in checks:
    print(('PASS' if ok else 'FAIL'), name)
    failed += 0 if ok else 1
print(f'VM Pass 3C-R1 persistent reclaim cursor static audit {len(checks)-failed}/{len(checks)} ' + ('PASS' if not failed else 'FAIL'))
raise SystemExit(1 if failed else 0)
