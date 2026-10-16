# Trinity AI R2 merge handoff

## Purpose

R2 prepares a provider-independent, capability-bounded action path for Trinity AI. FunctionGemma remains the native production target; Remote Ollama is an optional LAN development provider only.

## Merge order

1. Start from the R84-compatible Trinity source.
2. Apply `TRINITY_FUNCTIONGEMMA_PREP_MERGE_R1`.
3. Run this R2 `merge_check.py` or `apply_merge` script.
4. If the checker reports `MANUAL INTEGRATION REQUIRED`, stop and reconcile the named seam against the newer authoritative branch. Do not bypass the checker by blindly copying patched files.

## Important integration boundaries

- R2 does not change filesystem/exFAT/TFS implementation.
- R2 does not change Baofeng/mesh implementation.
- R2 does not change `desktop.cpp` or the Settings application implementation.
- `OpenSettings` currently launches the existing Settings app; section preselection is deferred to the later UI integration phase.
- Internet Radio actions refer to Trinity Internet Radio/media controls, not amateur-radio hardware.
- The R2 audit ring is RAM-only.

## Remote provider

Remote Ollama is disabled until explicitly configured. AISVC owns at most one remote fetch request. The endpoint is bounded to a host/model/port configuration and `/api/chat` with `stream:false`. Only exact known `trinity_*` tool names can become action requests.

## Local provider

The Local FunctionGemma provider seam exists, but R2 deliberately reports `RuntimeUnavailable`. The next phase is the native ggml/llama CPU runtime, not embedding Ollama into Trinity.

## Verification

See `VERIFICATION_R2.txt` for exact build hashes and qualification results.
