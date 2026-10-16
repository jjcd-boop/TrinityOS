# Trinity AI R2 Merge Handoff

## Baseline

This R2 package applies on top of the previously qualified **FunctionGemma Prep Merge R1**. The automatic merge path verifies R1 evidence and exact hashes for every existing-file R2 integration seam before changing the target tree. Drift produces `MANUAL INTEGRATION REQUIRED`.

## Result

R2 establishes:

`AI provider -> validated fixed action -> kernel Action Broker -> existing Trinity service -> bounded result -> RAM audit`

The provider does not receive raw OS authority. The broker validates caller identity, provider origin, action identifier, permission class and bounded arguments before invoking a typed Trinity adapter.

## Remote Ollama

`RemoteOllama` is an optional LAN **development provider only**. Trinity sends a bounded HTTP `/api/chat` request to an explicitly configured endpoint. No Ollama executable, Go runtime, Ollama model store or Ollama model payload is included.

Responses may produce at most one allow-listed action. Unknown tools and multiple tool calls are rejected before broker execution.

## Native FunctionGemma

R1's GGUF/model discovery foundation remains intact. R2 includes the `LocalFunctionGemma` provider identity but intentionally returns `RuntimeUnavailable`; native ggml/llama inference is a later phase.

## Capability boundary

AISVC receives only the existing AI/runtime capabilities plus bounded file-range reads and the three fetch syscalls needed by the optional remote provider. It is not granted filesystem-write, raw network-control, process-control, USB/device-control, audio-control or power-control authority.

The 128-entry audit ring is RAM-only and protected by an atomic lock. Actions are fixed enums rather than arbitrary paths or shell strings.

## Interaction with active branches

R2 does not modify the exFAT/filesystem implementation, Desktop/Search/Start assistant UI, or Baofeng/amateur-radio mesh implementation. Internet Radio broker actions refer only to Trinity's existing Internet Radio/media service.

## Qualification

Final qualification used a fresh original R84 extraction, exact R1 merge, then the final R2 package. All R1/R2 tests, AISVC verification and the full Trinity build passed; the build reported `Artifact verification: PASS`.

Final package SHA-256:
`42fbaf4def2ef49cba72de7c950c964999aefe89aaee37ae6706fc8229906e79`

## Hardware boundary

A real RemoteOllama LAN exchange still requires hardware testing on Trinity against a reachable external Ollama server. Settings-section actions currently launch the existing Settings application; visible section deep-link refinement remains deferred to later UI integration.
