# Trinity AI R2 — Action Broker + Remote Provider

## What R2 adds

R2 adds the controlled operating-system action layer that sits between an AI provider and Trinity services. Models never receive a generic shell, arbitrary syscall execution, arbitrary file paths, or direct device authority.

The release includes:

- Versioned `AIActionRequest` / `AIActionResult` contracts.
- A kernel-side allow-listed AI Action Broker.
- A 128-entry in-memory audit ring.
- Read-only status actions for battery, network, network diagnostics, process-memory summary, broker status, and audit status.
- Bounded benign controls for approved app launch, approved folders, Settings launch, volume adjustment, Internet Radio play, and Internet Radio next.
- Provider abstraction for `LocalFunctionGemma` and optional `RemoteOllama`.
- A bounded Ring-3 remote Ollama `/api/chat` adapter using `stream:false` and exact known tool names.
- AISVC v2 IPC for provider configuration, prompt submission, provider polling/cancellation, action submission/results, broker status, and audit snapshots.
- Conflict-aware R2 merge tooling with preflight hashes, rollback copies, and refusal on source drift.

## Ollama is not part of Trinity

Ollama remains an optional development backend running on another computer over the LAN. R2 does not contain the Ollama executable, Go runtime, Ollama model store, or an Ollama model.

The production local direction remains:

`Trinity -> AISVC -> native ggml/llama runtime -> FunctionGemma`

Local FunctionGemma inference is intentionally still reported as `RuntimeUnavailable` in R2. Native inference is the next runtime phase.

## Security boundary

The model can request only enumerated actions. R2 exposes no privileged/destructive action. Privileged requests are denied by the broker, unknown remote tool names are rejected before the broker, and provider-originated actions must come from `process::Id::AiService` with a matching provider identity.

Browser networking is not widened: Browser POST remains capped at 768 bytes. AISVC alone receives a separate 32 KiB request allowance for the bounded remote-provider request. The staging buffer is static/lock-protected rather than placed on the kernel stack.

## Branch isolation

R2 does not modify Trinity filesystem implementation, exFAT/TFS code, Baofeng/mesh code, Desktop implementation, or the Settings application implementation. The audit log is RAM-only in this release.

R2 requires the FunctionGemma R1 preparation layer first. Automatic application intentionally refuses to modify a tree if the R1 merge seams have drifted; this protects newer filesystem/desktop work from blind patching.

## Qualification

The exact merge package was qualified by applying exact R1 to a fresh R84 extraction and then applying R2. The R2 package passed its drift/refusal matrix, host tests under ASan/UBSan with warnings-as-errors, AISVC ELF verification, and a complete Trinity build with `Artifact verification: PASS`.

Hardware LAN communication with a real Ollama server remains a hardware-test item.

## Next phase

R3 should begin the native Trinity ggml/llama platform layer: allocator/alignment, CPU feature detection, worker threading, timing, cancellation/watchdog support, and the minimal CPU backend required to load and execute FunctionGemma without Ollama.
