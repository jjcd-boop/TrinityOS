# Trinity Navigator — H3C5 compact segmented-only DOM parser

**Status:** isolated C++20 source checkpoint; verified with GitHub Actions
host Clang and ASan/UBSan. **Not integrated into Trinity Navigator's
production R95 M1-or-later source; no EFI or hardware qualification.**

## Branch and changed files
Branch: \`prototype/html5-css-engine-h3c5\`
Base: \`prototype/html5-css-engine-h3c4\`

- \`include/trinity_html5_tree.hpp\`: build-time segmented-only configuration.
- \`src/trinity_html5_tree.cpp\`: accessors compiled without legacy fallback.
- \`tests/html_h3c5_compact_host.cpp\`: size assertions and live DOM tests.
- \`.github/workflows/trinity-html5-h3c5.yml\`: both configurations, existing
  regression matrix, and official HTML corpus survey.
- \`H3C5_REPORT.md\`: this report.

## What changed
The previous \`TreeBuilder(SegmentedNodeArena&)\` used a segmented store for
nodes but still embedded the old 384-node fallback array inside the parser
instance, wasting more than 600 KB of baseline memory.

H3C5 defines an **opt-in compile-time flag**:
\`TRINITY_HTML5_SEGMENTED_ONLY\`

When used consistently in **every translation unit** that includes/builds
\`TreeBuilder\`, it:
- removes the inline 384-node fallback from its class layout;
- deletes the legacy no-argument constructor;
- keeps explicit \`TreeBuilder(SegmentedNodeArena&)\` construction;
- routes node access directly to the caller-owned segmented arena;
- retains all existing parsing logic and error handling.

**Critical ABI constraint:** This macro changes the physical C++ object
layout. Mixed macro/non-macro object files MUST NOT be linked together.
Trinity should compile it as one isolated Ring-3 parsing service, with
a single build flag and a C ABI/IPC boundary; do not expose this C++
object directly across subsystem boundaries.

## Measured memory difference in host CI
- Legacy \`sizeof(TreeBuilder)\`: **612,152 bytes** (597.8 KiB).
- Segmented-only \`sizeof(TreeBuilder)\`: **8,504 bytes** (8.3 KiB).
- Base object reduction: **about 98.6%**, or 603,648 fewer bytes.
- This is \`sizeof(TreeBuilder)\` alone, *not total browser RAM*. The
  segmented arena separately allocates node pages subject to strict
  caller-set byte/node quotas.
- A separate optimization will be needed for the 8 KiB pending table
  text queue and formatting lists if reducing the parser object further.

## Tests
Successful CI before this report commit:
https://github.com/jjcd-boop/TrinityOS/actions/runs/39142740309

Both build modes:
- Compiled under C++20, \`-Wall -Wextra -Werror\`, ASan + UBSan.
- Parsed a live HTML document with **2,305 nodes** in the H3C5 test.
- Rebuilt a second small document and released all segments.
- Enforced out-of-range handle rejection and base-layout size assertions.

Legacy validation:
- H3C4's 2,605-node/byte-budget/OOM/parity tests PASS.
- All H1 tokenizer/CSS geometry, H2 DOM, H3A table, H3B1/B2 formatting,
  H3C1 context, and H3C2/H3C3 regression tests PASS.

Segmented-only:
- \`-DTRINITY_HTML5_SEGMENTED_ONLY\` used for every related C++
  source and test translation unit.
- Full \`-ffreestanding -fno-exceptions -fno-rtti\` syntax check PASS.
- No hidden fixed-node fallback allocation.

The exploratory pinned 240-case official corpus remains at **187 matches,
44 mismatches, 2 parser rejections, 7 unsupported comparison cases**.
H3C5 changes memory layout only; it does not claim to fix standards gaps.

## Limitations and next pass
- Segmented node handles are still 16 bits and the arena limit is 8,192
  nodes. Larger real-world documents need a 32-bit handle upgrade and a
  more compact TreeNode/attribute representation.
- The segmented-only configuration is not a dynamic runtime switch;
  the legacy and compact layouts are build-time distinct ABIs.
- No CSS cascade, style matching, dynamic DOM mutations, QuickJS bridge,
  paint/layout adapter, GPU/image decoder hookup, real network fetching,
  or EFI integration were performed here.
- C1 CSS tokenizer/declaration parser proceeds on **separate GitHub branch**
  \`prototype/css-engine-c1\`, avoiding premature branch merges.

**Next:** C2 selector parsing, specificity and cascade, then H4A read-only
DOM-to-computed-style integration using compact Ring-3 parser storage.
Continue HTML official-corpus repairs independently and integrate only
against the exact authoritative modern Trinity full source.

References:
https://html.spec.whatwg.org/multipage/parsing.html
