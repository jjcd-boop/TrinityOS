# Trinity AI Action Broker / Remote Provider R2

**Final qualification:** 2026-10-16  
**Baseline:** Trinity R84 + qualified FunctionGemma Prep Merge R1  
**Delivery:** conflict-aware merge package; not a replacement source tree

R2 establishes the bounded path the future native FunctionGemma runtime will use:

`AI provider -> validated fixed action -> kernel AI Action Broker -> existing Trinity service -> bounded result -> RAM audit`

It also includes an **optional remote Ollama LAN development provider**. Ollama is not installed into Trinity: no Ollama executable, Go runtime, model store, or Ollama model payload is shipped. `LocalFunctionGemma` remains the production-facing provider identity and intentionally returns `RuntimeUnavailable` until native ggml/llama inference is ported.

## R2 capabilities

- Versioned fixed-size AI action ABI.
- Kernel Action Broker with provider/caller/permission/argument validation.
- 128-entry RAM-only audit ring with atomic synchronization.
- Fixed adapters to approved app, power, network, audio and Internet Radio services.
- AISVC-only bounded fetch ownership.
- Browser POST remains capped at 768 bytes; AISVC receives a separate 32 KiB allowance.
- Remote `/api/chat` provider with `stream:false`, one request at a time, exact allow-listed tool mapping, and rejection of unknown or multiple tool calls.
- AISVC v2 IPC with bounded prompt/result mailboxes.
- Transport failure/cancel recovery that clears busy state.

## Deliberate exclusions

No arbitrary shell/command execution, arbitrary executable/path access, filesystem mutation, raw network/device control, shutdown/reboot action, Desktop/Search/Start assistant UI, exFAT implementation change, or Baofeng/amateur-radio mesh change is included in R2.

## Final qualification

A fresh original R84 tree was qualified by applying the exact R1 package and then the final R2 package. R1 regressions, R2 ASan/UBSan/security/static tests, AISVC ELF verification, and the full Trinity build passed. The final build reported `Artifact verification: PASS`.

Final merge ZIP SHA-256:

`42fbaf4def2ef49cba72de7c950c964999aefe89aaee37ae6706fc8229906e79`

See `VERIFICATION_R2.txt`, `AI_R2_MERGE_HANDOFF.md`, and `FINAL_REVIEW_R2.txt` for details.

The real remote-Ollama LAN round trip remains a hardware validation item; native FunctionGemma inference is the next major AI-runtime phase.
