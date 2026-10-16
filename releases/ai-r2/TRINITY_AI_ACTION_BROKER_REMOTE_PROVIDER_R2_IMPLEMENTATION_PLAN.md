# Trinity AI Action Broker + Remote Provider R2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the R1 AISVC layer with a bounded AI Action Broker, in-memory audit ring, provider abstraction, and optional remote Ollama development provider while preserving FunctionGemma as Trinity's local production target.

**Architecture:** Provider output remains untrusted Ring-3 data. AISVC converts only recognized provider tool calls into a fixed `AIActionRequest`; the kernel broker validates caller, action, permission class and arguments, executes only allow-listed adapters, and emits a fixed result plus audit entry. Remote Ollama uses the existing Trinity fetch broker; no Ollama binary/runtime/model enters Trinity.

**Tech Stack:** Trinity freestanding C++20-style codebase, fixed-size IPC structs, Ring-3 AISVC, kernel GUI runtime/capability broker, existing HTTP/HTTPS fetch transport, host C++ tests, Python static/merge tests.

**Spec:** `docs/TRINITY_AI_ACTION_BROKER_REMOTE_PROVIDER_R2_DESIGN.md`

## Global Constraints

- Baseline is TrinityOS R84 with `TRINITY_FUNCTIONGEMMA_PREP_MERGE_R1` already applied.
- Do not redesign Desktop UI, Start, Search, compositor, widgets, or expanded-desktop layout.
- Do not redesign exFAT/TFS persistence or add persistent AI history/audit storage.
- Do not embed FunctionGemma/GGUF or Ollama payloads in EFI/source/merge ZIP.
- Ollama is optional remote development infrastructure only; local FunctionGemma remains provider identity `LocalFunctionGemma`.
- AISVC stays Ring 3 and gains only `NetworkFetchStart`, `NetworkFetchPoll`, and `NetworkFetchCancel` beyond R1 capabilities.
- No arbitrary path, executable path, syscall number, pointer, shell command, source execution, shutdown/reboot, file mutation, driver mutation, or privileged action is accepted.
- One provider request and one provider-originated action may be active per AISVC instance.
- Remote limits: prompt <= 8 KiB, request body <= 32 KiB, response <= 128 KiB, model name <= 64 bytes, endpoint host <= 96 bytes.
- R1 model probing/status remains compatible and all R1 tests stay green.
- Merge package must preflight all touched existing-file anchors and stop with `MANUAL INTEGRATION REQUIRED` before editing on drift.

## Review Focus

1. **Forged provider/tool data:** malformed IDs, wrong permission class, or unknown tool names must be rejected before any adapter runs; pinned by Tasks 1 and 5.
2. **Remote endpoint abuse:** credentials, fragments, oversized host/model/prompt, or host/port mismatch must not reach `NetworkFetchStart`; pinned by Task 4.
3. **Fetch ownership leakage:** cancellation, failure, and timeout must leave no AISVC-owned fetch slot active; pinned by Tasks 3 and 4.
4. **Privilege drift:** AISVC must not gain `NetworkControl`, file-write, process-control, audio raw PCM, or device-control capabilities; pinned by Task 6.
5. **Merge drift:** changes in the user's newer Desktop/filesystem/network source must cause a fail-before-edit merge result, never partial application; pinned by Task 8.

---

## File Structure

### New files

- `include/trinity/ai_action_protocol.hpp` — fixed action/result/audit/provider ABI.
- `include/trinity/ai_action_broker.hpp` — broker validation/execution interface and adapter hooks.
- `src/kernel/ai_action_broker.cpp` — broker, audit ring, pure validation.
- `include/trinity/ai_provider.hpp` — provider request/result/endpoint contracts.
- `src/user/ai_provider.cpp` — provider-independent tool schema and local provider stub.
- `src/user/ai_provider_ollama.cpp` — bounded Ollama request builder/response parser/state machine.
- `tests/ai_r2_action_broker_host.cpp` — protocol/broker/audit host tests.
- `tests/ai_r2_provider_host.cpp` — tool schema, JSON parsing, endpoint, request-size host tests.
- `tests/ai_r2_integration_static.py` — capabilities, IPC, build, no-payload regression checks.
- `tests/ai_r2_merge_checker.py` — merge refusal matrix.

### Existing files modified

- `include/trinity/gui_ipc.hpp` — append R2 AI worker/client commands and bounded payload fields; increment AI IPC version only.
- `include/trinity/gui_runtime.hpp` — add narrow broker adapter declarations, not UI APIs.
- `src/kernel/gui_runtime.cpp` — AI IPC bridge, AISVC fetch capability, bounded launch/radio adapters.
- `src/kernel/syscall.cpp` — admit AISVC as bounded fetch owner/liveness identity only.
- `userland/aisvc_entry.cpp` — provider/broker service loop and fetch syscall adapter.
- `src/user/ai_service.cpp` / `include/trinity/ai_service.hpp` — retain R1 model status while adding provider/action state.
- `scripts/build_aisvc_elf.sh` — include provider sources.
- `scripts/build.sh` — compile/link broker source into normal kernel build; no model/Ollama payload.
- `src/user/apps/settings.cpp` — only if required for Settings section deep-link consumption; no layout/UI redesign.

---

### Task 1: Fixed AI Action Protocol, Validation, and Audit Ring

**Files:**
- Create: `include/trinity/ai_action_protocol.hpp`
- Create: `include/trinity/ai_action_broker.hpp`
- Create: `src/kernel/ai_action_broker.cpp`
- Create: `tests/ai_r2_action_broker_host.cpp`

**Interfaces:**
- Produces `trinity::ai_action::ContractVersion = 1`.
- Produces enums `ProviderId`, `PermissionClass`, `ActionId`, `Disposition`, `AppTarget`, `FolderTarget`, `SettingsSection`.
- Produces trivially-copyable `Request`, `Result`, `AuditEntry`, `AuditSnapshot` with no pointers or variable arrays.
- Produces `ai_action_broker::execute(const ai_action::Request&, const Context&, const Hooks&) noexcept` and `audit_snapshot() noexcept`.
- `Context` carries provider ID, caller process ID, and `provider_originated` flag; no kernel pointers.
- `Hooks` exposes only typed functions for launch/open/settings/power/network/process-memory/volume/radio.

- [ ] **Step 1: Write RED host tests for protocol layout and action validation.**

Tests must assert:
- valid `BatteryStatus`/`NetworkStatus` (`ReadOnly`) and `LaunchApp`/`AdjustVolume` (`BenignControl`) reach a fake hook;
- `Privileged`, unknown action IDs, mismatched permission classes, invalid enum targets, and volume deltas outside `-25..+25` do not call hooks;
- provider-originated request from caller other than `process::Id::AiService` is denied;
- every allowed/denied/invalid attempt appends exactly one audit entry;
- 128-entry ring wraps in sequence order without overwrite corruption.

- [ ] **Step 2: Run RED test.**

Run host compile/test command with `-Wall -Wextra -Werror -fsanitize=address,undefined`.
Expected: compile/link failure because R2 action protocol/broker does not exist.

- [ ] **Step 3: Implement the minimal fixed ABI and pure broker.**

Use integer/enum arguments only. `LaunchApp`, `OpenFolder`, and `OpenSettings` accept enum values only; no string/path fields exist in the kernel action request.

- [ ] **Step 4: Run GREEN host test and full R1 host suite.**

Expected: R2 action tests pass; all R1 layout/range/manifest/GGUF/state tests remain green.

- [ ] **Step 5: Commit as `feat(ai): add bounded action broker protocol` in the isolated implementation repo.**

---

### Task 2: Kernel Action Adapters to Existing Trinity Services

**Files:**
- Modify: `include/trinity/gui_runtime.hpp`
- Modify: `src/kernel/gui_runtime.cpp`
- Modify: `src/kernel/ai_action_broker.cpp`
- Extend test: `tests/ai_r2_action_broker_host.cpp`

**Interfaces:**
- Produces a `gui::AiActionAdapters`/equivalent narrow bridge used only by the broker.
- App mapping is exactly Writer=1, Calculator=2, Explorer=3, Settings=4, Browser=5, Media=6, Games=7, Sheets=9, Slides=10, Notes=11.
- Folder mapping is exact known launch paths: Home=`/`, Desktop=`/Users/Default/Desktop`, Documents=`/Documents`, Downloads=`/Downloads`.
- Settings mapping is fixed section enum; no provider string becomes an application path.
- Battery uses `power::read_snapshot`; network uses `network::snapshot` and existing qualification state; volume uses `audio::volume()` + `audio::control(7, clamped_absolute)`; radio reuses the existing `RadioIpc` command bridge.

- [ ] **Step 1: Add RED adapter-mapping tests.**

Assert every valid app/folder/settings enum maps to exactly one target and all out-of-range values fail without adapter execution.

- [ ] **Step 2: Run RED.**

Expected: mapping/adapter interfaces absent.

- [ ] **Step 3: Implement minimal kernel adapters.**

Launch/open actions must reuse the existing private-app launch/document path. Do not create a second app loader, Explorer, Settings, radio, network, or audio pipeline. If a Settings section cannot yet be consumed without a UI redesign, launch Settings and return a bounded `NotReady` detail for the deep-link while preserving the action contract; do not touch Settings layout.

- [ ] **Step 4: Run GREEN host/static tests.**

Expected: mappings exact, no arbitrary path/executable string path exists in action ABI, no duplicate subsystem implementation is introduced.

- [ ] **Step 5: Commit `feat(ai): connect action broker to Trinity services`.**

---

### Task 3: Extend AISVC Fetch Capability and Ownership Safely

**Files:**
- Modify: `src/kernel/gui_runtime.cpp`
- Modify: `src/kernel/syscall.cpp`
- Create/extend: `tests/ai_r2_integration_static.py`

**Interfaces:**
- AISVC syscall allow-list gains exactly `NetworkFetchStart`, `NetworkFetchPoll`, `NetworkFetchCancel` in addition to R1 `GuiRuntime`, `QueryUserCs`, `ReadFileRange`.
- `fetch_broker_caller()` recognizes `process::Id::AiService` as owner `gui::AiService` with current generation.
- `fetch_broker_owner_alive()` recognizes the AI slot and validates matching generation.
- AISVC owner limit is exactly one in-flight/queued request.

- [ ] **Step 1: Write RED static/host assertions.**

Assert AISVC has the three fetch syscalls and still lacks `NetworkControl`, `AudioControl`, `AudioPlayPcm`, `AudioQueuePcm`, `WriteFile*`, filesystem mutations, `ProcessControl`, shutdown/reboot, USB control, raw storage and driver/device control.

- [ ] **Step 2: Run RED static test.**

Expected: AISVC fetch permission/owner support missing.

- [ ] **Step 3: Implement only the fetch owner/capability extension.**

Do not alter HTTP/TCP/TLS/DHCP/DNS transport logic.

- [ ] **Step 4: Run GREEN static tests plus existing Browser fetch static/build checks.**

Expected: Browser/Desktop ownership behavior remains unchanged and AISVC is a third bounded owner class.

- [ ] **Step 5: Commit `feat(ai): admit bounded AISVC network fetch`.**

---

### Task 4: Provider Abstraction and Remote Ollama Parser/Request State

**Files:**
- Create: `include/trinity/ai_provider.hpp`
- Create: `src/user/ai_provider.cpp`
- Create: `src/user/ai_provider_ollama.cpp`
- Create: `tests/ai_r2_provider_host.cpp`

**Interfaces:**
- `ProviderId::LocalFunctionGemma` returns `RuntimeUnavailable` in R2.
- `ProviderId::RemoteOllama` is disabled until an in-memory `RemoteEndpoint` is explicitly configured.
- `RemoteEndpoint`: `enabled`, host <=96 bytes, port default 11434, model <=64 bytes; no credentials/userinfo or fragments.
- `ProviderRequest`: prompt <=8192 bytes; one request only.
- `ProviderResult`: plain text or one `ai_action::Request`, plus bounded provider status/error.
- Builder emits `POST /api/chat`, `Content-Type: application/json`, `stream:false`, model, one user message, and deterministic Trinity `tools` schema.
- Parser recognizes one Ollama tool call only and maps exact tool names to fixed action enums.

- [ ] **Step 1: Write RED provider tests.**

Cover disabled provider/no network request, valid host/port/model, rejected credentials/fragments/oversized fields, request <=32 KiB, valid plain-text response, valid one-tool response, unknown tool, malformed JSON, invalid/oversized tool argument, >128 KiB response, and second simultaneous request -> `Busy`.

- [ ] **Step 2: Run RED under ASan/UBSan.**

Expected: provider symbols absent.

- [ ] **Step 3: Implement local stub, deterministic tool schema, JSON escaping/parser, endpoint policy, and remote state machine without network syscalls.**

Parser is purpose-built/bounded; do not pull QuickJS or another general runtime into AISVC for JSON.

- [ ] **Step 4: Run GREEN provider tests warning-clean under sanitizers.**

- [ ] **Step 5: Commit `feat(ai): add provider abstraction and Ollama protocol`.**

---

### Task 5: Extend AI IPC and AISVC Service Loop

**Files:**
- Modify: `include/trinity/gui_ipc.hpp`
- Modify: `include/trinity/ai_service.hpp`
- Modify: `src/user/ai_service.cpp`
- Modify: `userland/aisvc_entry.cpp`
- Modify: `src/kernel/gui_runtime.cpp`
- Modify: `scripts/build_aisvc_elf.sh`
- Extend: `tests/ai_r2_integration_static.py`

**Interfaces:**
- Increment `AiServiceIpcVersion` from 1 to 2; retain R1 Ensure/Retire/Status/ProbeModel semantics.
- Add bounded commands: provider configure, provider submit, provider poll/cancel, action submit/poll, broker status, audit snapshot.
- Only AISVC may submit `provider_originated=true` broker requests.
- Desktop/Settings/Console diagnostic clients may query status/audit and issue deterministic non-provider test actions, but may not impersonate provider-originated AISVC requests.
- AISVC uses syscall adapters for fetch start/poll/cancel and never blocks/spins on Desktop UI thread.

- [ ] **Step 1: Write RED IPC/state tests/static assertions.**

Assert v1 model status still represented in v2, invalid version rejected, non-AISVC provider action denied, one action/request active, cancellation returns service to reusable idle state, and audit snapshot is bounded.

- [ ] **Step 2: Run RED.**

Expected: v2 commands/state absent.

- [ ] **Step 3: Implement v2 IPC and AISVC loop.**

R1 probing remains callable. Remote provider builds request, uses existing fetch syscalls, polls bounded response chunks into <=128 KiB AISVC storage, parses one response, and submits at most one broker action.

- [ ] **Step 4: Build AISVC ELF and run verification.**

Expected: fixed entry remains `0x40000000`; no unresolved symbols; no Ollama/GGUF bytes linked; model probing still works.

- [ ] **Step 5: Commit `feat(ai): wire providers and action broker through AISVC`.**

---

### Task 6: Diagnostic Path and Security Regression Gate

**Files:**
- Prefer create: `tests/ai_r2_diagnostic_host.cpp` and/or a bounded Console diagnostic command file if existing Console command structure permits a low-conflict append.
- Modify only if needed: `src/user/shell/console.cpp`
- Extend: `tests/ai_r2_integration_static.py`

**Interfaces:**
- Diagnostic flow can Ensure AISVC, inspect R1 model status, set temporary remote endpoint/model, submit one prompt, poll result, inspect one action result, and retrieve audit status.
- No Start/Search/Desktop UI changes.

- [ ] **Step 1: Write RED end-to-end deterministic diagnostic test using a fake provider/fetch adapter.**

Exercise `provider -> tool request -> broker -> fake existing service -> result -> audit` and denied action path.

- [ ] **Step 2: Run RED.**

- [ ] **Step 3: Implement the smallest diagnostic surface.**

Prefer host/static diagnostic harness over production GUI. If Console integration is added, use explicit commands such as `AI STATUS`, `AI AUDIT`, and `AI TEST <fixed-id>`; do not add free-form prompt UI in R2.

- [ ] **Step 4: Run GREEN regression/security suite.**

Assert no Search/Start/Desktop UI modifications, no file writes from AISVC, no `NetworkControl`, no privileged actions, and no provider payload persistence.

- [ ] **Step 5: Commit `test(ai): add R2 diagnostic and security qualification`.**

---

### Task 7: Build Integration and Full R84+R1+R2 Qualification

**Files:**
- Modify: `scripts/build.sh`
- Extend: `tests/ai_r2_integration_static.py`

**Interfaces:**
- Kernel build includes `src/kernel/ai_action_broker.cpp`.
- AISVC build includes provider sources.
- EFI/USB build never copies `*.gguf`, Ollama executable, Go runtime, or Ollama model store.

- [ ] **Step 1: Write RED build/static checks for source inclusion and payload exclusion.**

- [ ] **Step 2: Run RED.**

- [ ] **Step 3: Add only required source compilation/link entries.**

- [ ] **Step 4: Run complete qualification on a fresh R84 extraction with R1 then R2 applied.**

Required evidence:
- all R1 host tests green;
- all R2 broker/provider/security tests green with warnings-as-errors;
- sanitizer suites green;
- AISVC ELF verifies with no unresolved symbols;
- full Trinity `scripts/build.sh` exits 0;
- artifact verification reports PASS;
- zero GGUF/Ollama payloads in normal build outputs.

- [ ] **Step 5: Commit `build(ai): integrate R2 broker and provider layer`.**

---

### Task 8: Produce Conflict-Aware R2 Merge Package

**Files:**
- Create package tree `TRINITY_AI_ACTION_BROKER_REMOTE_PROVIDER_R2/` with `overlay/`, `patches/`, `tests/`, `tools/ai_merge/`, `rollback/`, `docs/`, `README.md`, `AI_R2_MERGE_HANDOFF.md`, `VERIFICATION_R2.txt`.
- Create `tools/ai_merge/merge_check.py`, `apply_merge.sh`, `apply_merge.ps1`.
- Create `tests/ai_r2_merge_checker.py`.

**Interfaces:**
- Checker first verifies R1 evidence (`ai_protocol.hpp`, AISVC slot/process identity, `ReadFileRange`, build integration) before R2 anchors.
- Existing-file patches and new-file overlay never overlap.
- All compatibility checks and `git apply --check` operations complete before first edit.

- [ ] **Step 1: Write RED merge-checker matrix.**

Fixture starts from included R1-qualified pristine/rollback state. Independently mutate each R2 patch anchor (AI IPC, GUI capability block, fetch ownership block, AISVC entry/build block, build source list) and assert `MANUAL INTEGRATION REQUIRED` with no file changes.

- [ ] **Step 2: Run RED.**

- [ ] **Step 3: Generate overlay, two-or-more focused patches, rollback copies/checksums, apply scripts, docs and verification record.**

Keep any Desktop/Settings integration in its own patch if it touches files likely to overlap the user's active branches.

- [ ] **Step 4: Apply exact package from zero to a second fresh R84 extraction, apply R1 first, then R2.**

Run checker -> dry-run -> apply -> R1+R2 tests -> AISVC build -> full Trinity build.

- [ ] **Step 5: Zip and verify archive.**

Required final checks: ZIP integrity PASS, SHA-256 recorded, no GGUF/Ollama/Go payload, docs/spec/plan included, and package size remains merge-layer scale rather than model scale.

- [ ] **Step 6: Commit `chore(ai): package R2 action broker remote provider merge`.**

---

## Completion Boundary

R2 is complete only when:

1. `LocalFunctionGemma` exists and truthfully reports `RuntimeUnavailable`.
2. `RemoteOllama` is disabled by default and can issue one bounded `/api/chat` POST only after explicit in-memory configuration.
3. One valid Ollama tool call maps to one fixed `AIActionRequest`.
4. Kernel broker validates every request and no dynamic function/syscall/path dispatch exists.
5. Allowed action reaches an existing Trinity subsystem adapter and produces a structured result.
6. Invalid/denied attempts generate audit entries and cannot execute.
7. AISVC owns at most one fetch and cancellation/failure releases ownership.
8. No privileged/destructive actions exist.
9. R1 model probe/status remains green.
10. Full R84+R1+R2 build and artifact verification pass.
11. Merge artifact contains no model/Ollama payload and refuses source drift before editing.
