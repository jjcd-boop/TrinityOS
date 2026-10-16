# Trinity AI Action Broker + Remote Provider R2 Design

**Date:** 2026-10-15  
**Baseline:** TrinityOS R84 + `TRINITY_FUNCTIONGEMMA_PREP_MERGE_R1`  
**Deliverable type:** merge-only package; not a replacement source tree

## 1. Purpose

R2 turns the R1 AI bring-up service into a safe, testable control plane without making Ollama part of Trinity. FunctionGemma remains the intended production local model. Ollama is an optional remote development provider used to exercise natural-language-to-tool-call behavior before native FunctionGemma inference is installed.

R2 must prove this end-to-end path:

`provider -> structured AI action -> kernel Action Broker -> allow-listed Trinity operation -> structured result -> audit record`

The model/provider must never receive a primitive equivalent to arbitrary syscall execution, shell execution, arbitrary process control, arbitrary file mutation, MMIO, port I/O, or raw kernel command execution.

## 2. Binding constraints

1. Do not modify the Desktop UI, Start menu, search bar, compositor, expanded-desktop work, or widget layout.
2. Do not redesign or mutate exFAT/TFS/filesystem persistence. R2 may only consume the R1 read-only `/System/AI` model namespace and existing APIs.
3. Do not embed FunctionGemma or any other GGUF model in EFI, source, AISVC, or the merge ZIP.
4. Do not include Ollama binaries, Go runtime, Ollama model store, or Ollama source in Trinity.
5. Native FunctionGemma remains the production target. Remote Ollama is development/test infrastructure and is disabled unless explicitly configured by a client.
6. AISVC remains Ring 3 and has no direct device authority.
7. All OS-changing operations requested by a provider pass through the kernel Action Broker.
8. Destructive or privileged actions are not implemented in R2.
9. R2 is mergeable on top of R1; it must refuse unsafe automatic application if stable integration anchors have drifted.
10. Existing R1 model validation, sparse AI arena, GGUF probe, and model placement remain compatible.

## 3. Existing R1 foundation reused by R2

R1 already provides:

- `process::Id::AiService`
- hidden GUI slot `gui::AiService`
- Ring-3 AISVC ELF
- 768 MiB sparse AISVC virtual arena
- 512 MiB resident cap
- `ReadFileRange`
- `/System/AI` read-only admission
- `ai_protocol.hpp`
- `ai_service.hpp`
- GGUF + manifest probing
- `Operation::AI`
- bounded AISVC status publication/polling
- AISVC build/embed path with no model bytes in EFI

R2 must extend these contracts rather than replace them.

## 4. Architecture

### 4.1 Components

R2 adds four distinct units:

1. **AI Action Protocol** — fixed-size versioned request/result structures shared by AISVC and the kernel broker.
2. **AI Action Broker** — kernel-side validator and executor for a small action allow-list.
3. **AI Provider Layer** — Ring-3 AISVC abstraction for local FunctionGemma and optional remote Ollama.
4. **AI Audit Ring** — bounded kernel-resident history of action requests and outcomes.

Logical flow:

```text
               Trinity client / diagnostic test
                           |
                           v
                        AISVC
                           |
               +-----------+------------+
               |                        |
       LocalAISVCProvider       RemoteOllamaProvider
       (runtime unavailable       (optional LAN HTTP)
        until native R3+)               |
               |                        |
               +-----------+------------+
                           |
                  AIActionRequest
                           |
                           v
                 Kernel AI Action Broker
                           |
            validate action + permission class
                           |
         +---------+--------+--------+---------+
         |         |        |        |         |
       Apps      Power   Network   Audio     Radio
         |         |        |        |         |
         +---------+--------+--------+---------+
                           |
                   AIActionResult
                           |
                append bounded audit entry
                           |
                           v
                         AISVC
```

### 4.2 Trust boundary

Provider output is untrusted input.

The Action Broker must validate:

- protocol version
- action ID
- permission class
- argument type/length/range
- allowed target app/folder/settings section
- caller identity (`AiService` only for provider-originated execution)
- action-specific state preconditions

The broker must never interpret provider-generated strings as source code, shell commands, syscall numbers, pointers, or executable paths.

## 5. AI Action Protocol

Create `include/trinity/ai_action_protocol.hpp`.

### 5.1 Contract

`ContractVersion = 1`.

All structures are fixed-size, trivially copyable, bounded, and contain no user pointers.

### 5.2 Permission classes

```text
ReadOnly
BenignControl
Privileged
```

R2 implements only `ReadOnly` and `BenignControl`.

Any `Privileged` request returns `Denied`/`NotImplemented` and is audited.

### 5.3 Initial action IDs

R2 implements the following broker actions:

**ReadOnly**

- `BatteryStatus`
- `NetworkStatus`
- `NetworkDiagnostics`
- `ProcessMemorySummary`
- `ActionBrokerStatus`
- `AuditSnapshot`

**BenignControl**

- `LaunchApp`
- `OpenFolder`
- `OpenSettings`
- `AdjustVolume`
- `RadioPlay`
- `RadioNext`

R2 explicitly does **not** implement:

- shutdown/reboot
- delete/rename/write files
- change Wi-Fi credentials
- change network configuration
- install applications
- modify drivers
- arbitrary process termination
- arbitrary process launch by path
- arbitrary filesystem path access
- kernel/debug commands
- Forge code execution
- compiler invocation

### 5.4 Argument restrictions

`LaunchApp` accepts only an enum of approved existing Trinity app identities; it does not accept executable paths.

Initial launchable set:

- Writer
- Calculator
- Explorer
- Settings
- Browser
- Media
- Games
- Sheets
- Slides
- Notes

`OpenFolder` accepts only an enum:

- Home
- Desktop
- Documents
- Downloads

No arbitrary path is accepted in R2.

`OpenSettings` accepts only an enum:

- General
- Network
- Power
- Mouse
- Display
- Audio
- AI

The `AI` deep link may resolve to a diagnostic placeholder until a full Settings UI is added later.

`AdjustVolume` accepts a signed delta clamped to `-25..+25` percentage points per action. It is not an absolute arbitrary mixer write.

`RadioPlay` and `RadioNext` reuse the existing single Desktop-owned radio transport. R2 must not instantiate a second radio/audio/network pipeline.

## 6. Kernel AI Action Broker

Create focused files:

- `include/trinity/ai_action_broker.hpp`
- `src/kernel/ai_action_broker.cpp`

The broker owns action validation/execution and must not be implemented as a large block directly inside `gui_runtime.cpp`.

`gui_runtime.cpp` is only the IPC/admission bridge.

### 6.1 Broker interface

Conceptual interface:

```cpp
Result execute(const Request& request, Context context) noexcept;
Status status() noexcept;
AuditSnapshot audit_snapshot() noexcept;
```

Exact types are defined by the implementation plan, but no broker interface may expose raw kernel pointers to AISVC.

### 6.2 Existing-service reuse

The broker must use existing Trinity control paths wherever available instead of introducing parallel implementations.

- Application launch: reuse GUI private-app launch queue/state machinery.
- Folder/open target: reuse app-start path/pending-document semantics where compatible; otherwise add one bounded enum-to-known-path translation at the broker/UI bridge.
- Battery/network state: reuse existing power/network snapshots.
- Network diagnostics: reuse existing qualification/status mechanisms; R2 must not write new DHCP/DNS/TCP implementations.
- Audio: reuse existing audio control path.
- Radio: reuse existing `RadioIpc` bridge.
- Process memory: build a read-only bounded summary from existing process/VM metadata; no process administration authority is granted.

## 7. Audit ring

Create a fixed-size in-memory audit ring owned by the Action Broker.

Requirements:

- 64 entries minimum; 128 preferred if the structure remains small.
- No heap allocation.
- Monotonic sequence number.
- Timestamp/TSC.
- Provider ID.
- Action ID.
- Permission class.
- disposition: allowed / denied / invalid / unsupported / failed.
- bounded numeric result/detail code.
- no full prompt text.
- no model response body.
- no passwords, tokens, cookies, request bodies, or arbitrary user document contents.

The ring is intentionally non-persistent in R2 to avoid collision with ongoing filesystem work.

## 8. Provider abstraction

Create:

- `include/trinity/ai_provider.hpp`
- `src/user/ai_provider.cpp`
- `src/user/ai_provider_ollama.cpp`

### 8.1 Provider IDs

```text
LocalFunctionGemma
RemoteOllama
```

`LocalFunctionGemma` exists as the production-facing provider identity but returns `RuntimeUnavailable` until the native inference runtime is installed in a later merge.

This means later native inference replaces provider internals rather than changing the Action Broker contract.

### 8.2 Provider request/output

A provider receives bounded text plus the fixed Trinity tool schema and returns either:

- plain text response
- one structured tool/action request
- provider error

R2 does not implement multi-step autonomous loops. One provider call may request at most one broker action. This deliberately prevents an early model/provider from chaining uncontrolled actions.

A later release may add bounded multi-action plans after individual actions are hardware-qualified.

## 9. Remote Ollama provider

Remote Ollama is development infrastructure only.

Ollama remains on another supported machine, e.g. Windows/Linux. Trinity communicates over the existing network-fetch transport.

Current Ollama API documentation exposes `POST /api/chat`, including an optional `tools` array for function/tool calling. R2 targets that API shape and sends `stream:false` for the first implementation so Trinity receives one bounded JSON response rather than implementing NDJSON streaming in the same pass.

### 9.1 Endpoint policy

There is no hardcoded Windows/ICS address.

The diagnostic client supplies an explicit endpoint configuration containing:

- IPv4 host or bounded hostname
- port (default may be 11434 when omitted)
- model name
- enabled flag

R2 must reject:

- embedded credentials/userinfo in URL
- URL fragments
- request URLs not matching the configured host/port
- responses above the configured bounded body limit

Default state is disabled.

### 9.2 Network privilege

AISVC currently receives only `GuiRuntime`, `QueryUserCs`, and `ReadFileRange` in R1.

R2 may add only:

- `NetworkFetchStart`
- `NetworkFetchPoll`
- `NetworkFetchCancel`

for AISVC.

It must **not** grant:

- `NetworkControl`
- raw NIC control
- DHCP control
- Wi-Fi credential authority
- general socket/MMIO authority

The existing fetch broker currently recognizes Desktop/Browser owners. R2 extends broker ownership/liveness checks to recognize the AI service as another bounded fetch owner. It must not change the network transport implementation itself.

### 9.3 Request limits

Initial remote-provider limits:

- one in-flight Ollama request owned by AISVC
- request body <= 32 KiB
- response body <= 128 KiB
- model name <= 64 bytes
- prompt <= 8 KiB
- endpoint host <= 96 bytes
- bounded timeout/cancel path

These are bring-up limits, not a promise of final assistant context size.

## 10. Tool schema

R2 contains a deterministic Trinity tool schema generated from the same action definitions the broker validates.

The remote provider must never invent an executable function name and have Trinity dispatch it dynamically.

Allowed tool names are exact mappings, for example:

```text
trinity_launch_app
trinity_open_folder
trinity_battery_status
trinity_network_status
trinity_network_diagnostics
trinity_open_settings
trinity_adjust_volume
trinity_radio_play
trinity_radio_next
trinity_process_memory_summary
```

Unknown tool/function names are rejected and audited as `Unsupported`.

Tool arguments are parsed into the fixed `AIActionRequest`; arbitrary JSON is never forwarded into kernel services.

## 11. IPC extension

R1 `Operation::AI` remains the single AI control operation.

R2 extends the AI IPC contract with bounded commands for:

- submit provider request
- poll provider result
- execute structured action
- poll action result
- read broker/audit status

The implementation should prefer appending new fields/commands while keeping R1 model-status semantics available. If the fixed structure must change size, increment `AiServiceIpcVersion` and update all R2 callers/worker code together.

Only these callers may inspect/administer AI state:

- AISVC worker
- Desktop diagnostic client
- Settings diagnostic client
- Console diagnostic client

Only AISVC may submit provider-originated broker actions.

## 12. Diagnostic client

R2 must not modify the production Search/Start/Desktop UI.

Instead it provides a small diagnostic/test path that can:

1. Ensure/start AISVC.
2. Inspect R1 model status.
3. Configure a temporary remote Ollama endpoint/model in memory.
4. Submit one bounded prompt.
5. Display/return provider status.
6. If a valid tool call is produced, send it through the Action Broker.
7. Retrieve the structured result and audit entry.

The diagnostic path may be console/static-test based if adding a graphical client would increase merge conflict risk.

## 13. Failure behavior

All AI functionality is optional.

Failure cases must never prevent Trinity boot or desktop admission.

Required deterministic failures include:

- AISVC missing -> AI unavailable
- local runtime unavailable -> `RuntimeUnavailable`
- remote provider disabled -> `ProviderDisabled`
- Ollama host unreachable -> `ProviderOffline`
- HTTP non-2xx -> `ProviderHttpError`
- response too large -> `ProviderResponseTooLarge`
- malformed JSON -> `ProviderInvalidResponse`
- unknown tool -> `UnsupportedAction`
- invalid tool arguments -> `InvalidArguments`
- privileged action -> `Denied`
- broker busy -> `Busy`
- target service unavailable -> `NotReady`

Cancellation must release the network-fetch ownership and leave AISVC reusable.

## 14. Resource/scheduling policy

Remote-provider parsing must not run on Desktop's UI thread.

AISVC remains a background-scheduled Ring-3 service.

R2 introduces no new permanent CPU pinning.

One provider request and one broker action may be active per AISVC instance. Queuing/multiple concurrent conversations are deferred.

## 15. Filesystem and model behavior

R2 does not change model placement.

FunctionGemma remains a one-time persistent USB asset:

`/System/AI/Models/functiongemma-270m-it-q8_0.gguf`

R2 does not require the model file for remote Ollama operation, and remote Ollama does not change or replace the local model configuration.

No AI audit or conversation data is written to disk in R2.

## 16. Merge packaging

Deliver:

`TRINITY_AI_ACTION_BROKER_REMOTE_PROVIDER_R2.zip`

Recommended structure:

```text
TRINITY_AI_ACTION_BROKER_REMOTE_PROVIDER_R2/
├── overlay/
├── patches/
│   ├── 0001-ai-action-core.patch
│   └── 0002-ai-provider-integration.patch
├── tests/
├── tools/ai_merge/
├── rollback/
├── docs/
├── README.md
├── AI_R2_MERGE_HANDOFF.md
└── VERIFICATION_R2.txt
```

The merge checker must require evidence that R1 is already present and verify anchors around every existing file R2 patches.

If Desktop/filesystem/GUI changes in the user's newer source have moved those anchors, the automatic merge must stop with `MANUAL INTEGRATION REQUIRED` before changing source files.

## 17. Tests and qualification

R2 requires test-first coverage for at least:

### Protocol/broker

- valid read-only action accepted
- valid benign action accepted
- privileged action denied
- unknown action rejected
- oversized/invalid arguments rejected
- non-AISVC provider-originated action rejected
- audit entry generated for allowed and denied requests
- ring wraps without corruption

### App/settings/folder mappings

- every accepted enum maps to exactly one intended target
- arbitrary executable/path strings cannot reach launch/open code

### Remote provider

- disabled provider makes no network request
- endpoint validation
- bounded POST body
- response body limit
- valid Ollama `/api/chat` response
- valid tool call -> fixed action
- unknown tool rejected
- malformed JSON rejected
- timeout/cancel releases request ownership
- one-request-at-a-time enforcement

### Regression/security

- AISVC still reads R1 model files
- R1 GGUF/manifest tests remain green
- AISVC still cannot call `NetworkControl`
- AISVC still cannot write files
- no Ollama binary/model stored in EFI/package
- normal Browser fetch path remains functional/static-compatible
- normal full Trinity build remains green

### Build

- warning-clean host tests
- AISVC ELF has no unresolved symbols
- full Trinity EFI build succeeds
- artifact verification passes
- merge applies to fresh R84+R1 fixture
- merge checker refuses deliberate drift cases

## 18. R2 success boundary

R2 is complete when all of the following are true:

1. AISVC can represent both `LocalFunctionGemma` and `RemoteOllama` providers.
2. Local FunctionGemma correctly reports runtime unavailable rather than pretending inference exists.
3. Remote Ollama can submit one bounded chat request through Trinity's existing fetch path when explicitly configured.
4. A valid remote tool call becomes a fixed `AIActionRequest`.
5. The kernel Action Broker accepts only allow-listed, validated actions.
6. An allowed action reaches the existing Trinity subsystem and returns a structured result.
7. Denied/invalid actions cannot bypass the broker.
8. Every broker attempt produces a bounded audit record.
9. No desktop/search UI integration is required.
10. No model/Ollama payload is embedded in normal Trinity build iterations.
11. Full Trinity build and R1 regression tests remain green.

## 19. Deferred to later merges

- native FunctionGemma/llama.cpp inference
- multi-tool autonomous loops
- Search/Start assistant UI
- persistent conversations/history
- persistent audit log
- privileged/destructive AI actions
- user-facing permissions UI
- model download UI
- voice input/output
- code-writing/execution agent
- Forge autonomous development integration

## 20. Design rationale

This R2 intentionally separates **reasoning/provider** from **authority**. Ollama can be swapped out, FunctionGemma can later become local, and neither change affects the kernel action contract. The model chooses only among explicit tools; Trinity remains the authority that validates and executes them.

This makes the remote Ollama phase useful rather than throwaway: it validates the exact provider-to-action path that native FunctionGemma will later use, while preserving Trinity's Ring-3 and capability-oriented direction.
