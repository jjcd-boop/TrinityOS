# Trinity Navigator — HTML5 H3C4: Live Segmented DOM TreeBuilder

**Checkpoint status:** GitHub-hosted C++20 experimental parser, host tests
PASS with AddressSanitizer and UndefinedBehaviorSanitizer. **Not merged into
the authoritative Trinity R95 M1-or-later source; no production EFI was built,
no x86-64 Trinity runtime or hardware qualification performed.**

## Source
Branch: \`prototype/html5-css-engine-h3c4\`
Base: \`prototype/html5-css-engine-h3c3\`

Changed:
- \`experiments/html5_css_h1/include/trinity_html5_tree.hpp\`
- \`experiments/html5_css_h1/src/trinity_html5_tree.cpp\`
- \`experiments/html5_css_h1/include/trinity_html5_segmented_nodes.hpp\` (documentation)
- \`experiments/html5_css_h1/tests/html_dynamic_tree_h3c4_host.cpp\` (new)
- \`.github/workflows/trinity-html5-h3c4.yml\` (new)
- \`experiments/html5_css_h1/H3C4_REPORT.md\` (new)

Previously staged segmented arena implementation remains in:
- \`experiments/html5_css_h1/src/trinity_html5_segmented_nodes.cpp\`

## Live TreeBuilder migration
- Backwards-compatible default \`TreeBuilder()\` retains its existing fixed
  **384-node** arena and API, so older tests and callers remain unchanged.
- The new explicit \`TreeBuilder(SegmentedNodeArena&)\` constructor uses a
  caller-owned segmented allocator. This mode shares *all existing*
  tokenizer, DOM construction, foster parenting and formatting behavior
  with the fixed mode; no forked parser algorithm.
- Storage reads/writes route through an indexed node adapter, and newly
  created nodes go through \`allocate_node\` with hard node/byte quotas.
- \`get()\` and name-based traversal return node handles/pointers from the
  selected backend, including nodes beyond index 384.
- \`reset()\` on each \`build()\` discards previous segmented allocations
  and reinserts the document root. Failed allocation is surfaced as
  \`BuildError::NodeLimit\`, \`MemoryBudget\`, or \`AllocatorFailure\`.
- The store is **borrowed** from its caller. Its lifetime must enclose the
  TreeBuilder lifetime; do not clear or destroy it during an active parse.
- All node IDs currently remain 16-bit indices; the segmented storage
  upper bound is **8,192 nodes** (further reduced by caller quotas).
- The fixed fallback array still remains embedded in TreeBuilder even
  when segmented mode is enabled, so this pass increases the *maximum
  document size* but does not yet minimize TreeBuilder's base object size.

## Qualification evidence
Successful source test run before this report:
https://github.com/jjcd-boop/TrinityOS/actions/runs/39141162767

- Actual HTML document containing **2,605 nodes** (document, doctype,
  html/head/body, 1,300 divs and 1,300 text nodes): PASS.
- Document rebuild back down to 9 nodes: PASS.
- Parser behavior under node-count quota, byte-budget exhaustion,
  and injected Ring-3 allocation failure: PASS.
- Legacy 384-node parser still fails closed when over capacity: PASS.
- **10 different HTML documents** produced identical complete node
  records (links, text, attributes, implied flags) with fixed and
  segmented backends: PASS.
- Freestanding-style Clang C++20 syntax for all parser modules:
  PASS (\`-ffreestanding -fno-exceptions -fno-rtti -Wall -Wextra -Werror\`).
- Existing H1 tokenizer/CSS, H2 DOM, H3A table foster, H3B1/B2 formatting,
  H3C1 table scope and H3C2/H3C3 tests under ASan+UBSan: PASS.
- Pinned official historical HTML corpus (240 examined) ran successfully;
  conformance gaps remain from H3C3. This survey is **not** a passing
  full-HTML5 standards suite, and reported mismatches are non-blocking.

## Future integration conditions
1. First reduce the fixed fallback footprint for production segmented mode
   (e.g., caller-provided backing and separate builder object) and evaluate
   32-bit node indices / more efficient per-node attribute representations.
2. Integrate only against an **exact authoritative full Trinity R95 M1-or-later
   source tree**, not an outdated GitHub \`main\` snapshot. Do not overwrite
   concurrent filesystem, USB, AX200, Realtek, Baofeng, AI or kernel work.
3. Run parser in an isolated Ring-3 service, with capability-provided
   bounded allocation callbacks, IPC ownership and crash containment.
4. Develop auditable DOM-to-renderer and JS/QuickJS bridges, then connect
   the retained CSS layout and hit boxes.
5. Qualify real Trinity C++ ABI, linker, ELF loader, production EFI,
   and physical hardware. Only then label this ready for desktop.

## Next recommended phase (H3C5 / H4A)
- H3C5: remove the unused inline 384-node fallback from production
  segmented-builder memory layout while preserving fixed-mode tests.
  Recheck base RAM cost, allocator behaviour and build-size admission.
- H4A: begin a read-only retained DOM-to-CSS-box adapter using this
  segmented storage, initially off-screen and without changing painting.
- Continue official HTML parsing corpus fixes independently;
  240 sampled inputs are not equivalent to complete HTML5 compliance.

Design references:
https://html.spec.whatwg.org/multipage/parsing.html
https://github.com/web-platform-tests/wpt/tree/master/html/syntax/parsing
