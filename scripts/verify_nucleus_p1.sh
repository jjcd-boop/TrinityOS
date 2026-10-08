#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
CXX="${HOST_CXX:-clang++}"
"$CXX" -std=c++20 -O2 -Wall -Wextra -Werror -I"$ROOT/include" \
  "$ROOT/tests/nucleus_health_p1_test.cpp" "$ROOT/src/nucleus/health.cpp" \
  -o "$OUT/nucleus_health_p1"
"$OUT/nucleus_health_p1"
python3 - "$ROOT" <<'PY'
import pathlib,sys
root=pathlib.Path(sys.argv[1])
checks={
 'src/kernel/scheduler.cpp':['SubsystemId::Scheduler','health_poll()'],
 'src/kernel/gui_runtime.cpp':['SubsystemId::Desktop','SubsystemId::Compositor','SubsystemId::Input'],
 'src/nucleus/nucleus.cpp':['health_register_pass1_defaults','SubsystemId::Kernel'],
 'src/user/shell/console.cpp':['GetNucleusHealth','NUCLEUS P1 health'],
 'src/kernel/syscall.cpp':['handle_get_nucleus_health','GetNucleusHealth'],
}
for rel,needles in checks.items():
    text=(root/rel).read_text()
    for needle in needles:
        if needle not in text:
            raise SystemExit(f'NUCLEUS P1 STATIC FAIL: {rel} missing {needle}')
print('NUCLEUS P1 STATIC PASS')
PY
