# Trinity Navigator — H3C3 Official HTML Corpus + Segmented DOM Arena

**Status:** Isolated C++ development checkpoint built and tested on GitHub
Actions. Not merged into production R95 M1 browser; no Trinity EFI generated
or actual hardware qualification.

## Source
Branch: \`prototype/html5-css-engine-h3c3\`
Parent checkpoint: \`prototype/html5-css-engine-h3c2\`

New files:
- \`tests/html5lib_official_corpus_h3c3.py\`
- \`tests/html_segmented_arena_h3c3_host.cpp\`
- \`tests/html_block_closure_h3c3_host.cpp\`
- \`include/trinity_html5_segmented_nodes.hpp\`
- \`src/trinity_html5_segmented_nodes.cpp\`
- \`.github/workflows/trinity-html5-official-corpus-h3c3.yml\`
- This report.

Updated:
- \`src/trinity_html5_tree.cpp\`

## Official corpus provenance
The upstream \`html5lib/html5lib-tests\` project removed its old
\`tree-construction/*.dat\` directory on June 26, 2026 because ongoing
parsing tests moved to Web Platform Tests. For reproducible comparison,
GitHub Actions retrieves eight official historical .dat fixture files from
the repo's pinned commit:
\`9329e64694e7835d0dcff9811e22856ef6ad16f9\`

https://github.com/html5lib/html5lib-tests/tree/9329e64694e7835d0dcff9811e22856ef6ad16f9/tree-construction

Inputs are drawn directly from those files, with test-only \`html5lib==1.1\`
providing the reference element/text tree. \`#document\` expected files,
attributes, namespaced elements, comments, parse-error counts, scripting,
fragment tests and styling **are not yet fully compared**.

## Measured results (after H3C3 block repair)
**240 official historical corpus cases examined**:
- **187** element/text tree-shape matches against Python html5lib.
- **44** different element/text tree shapes.
- **2** parser rejections (must be analyzed, not marked passing).
- **7** unsupported comparison cases: 6 non-body reference documents and
  1 fragment-mode case.
- In the sampled \`blocks.dat\`, **48/48 matched** after specific fixes.

Initial unmodified H3C2 against same 240 inputs:
181 matches / 50 mismatches / 2 rejected / 7 unsupported.

The 6 net match improvements came from proper implied closing \`<p>\` for
\`center\`, \`dir\`, \`fieldset\`, \`menu\`, \`search\`,
\`summary\`, and \`listing\` start tags. The reference
also showed that \`dialog\` MUST NOT follow the same shortcut in this
context; a regression test now enforces that distinction.

The corpus survey is **diagnostic**, not a conformance badge. CI must
fail on fixture-access, collection counts or harness/runtime errors, but
mismatches are currently reported (not all cause CI failure) because the
parser is still under development. Full conformance requires comparison
to official \`#document\` expectations and more test modes.

## New segmented DOM node arena
A separate allocation-free-API C++ component:
- Stable 16-bit handles, eight nodes per allocated segment.
- Up to 1024 segments / 8192 total nodes (guarded by a caller-set node quota).
- Explicit caller-set byte budget. Supports partial final segment usage.
- Caller-provided aligned allocate/release callbacks suitable for a future
  capability-constrained Ring-3 parser service.
- No global allocator use in the component; placement construction only
  on memory already granted by the caller.
- Atomic failure for new-segment allocation: the previously allocated node
  handles remain valid if a quota or allocator failure occurs.
- Clear/destroy returns all segments and preserves no stale handles.
- Host tests append **2,100 nodes**, plus node-budget, byte-budget, OOM and
  clear/reuse tests, under AddressSanitizer and UndefinedBehaviorSanitizer.

**Integration caveat:** the actual \`TreeBuilder\` still uses its original
fixed 384-node array. The segmented arena is an independent qualified
prototype. It is not a completed dynamic-memory migration.

## CI qualification
Source tests include Clang C++20 \`-ffreestanding -fno-exceptions
-fno-rtti -Wall -Wextra -Werror\` syntax checks, host ASan/UBSan arena and
parser smoke tests, a 240-input official corpus survey, and a downloadable
JSON artifact containing exact counters and sample mismatches.

Successful source run before report commit:
https://github.com/jjcd-boop/TrinityOS/actions/runs/39140272964

Corpus JSON artifact:
https://github.com/jjcd-boop/TrinityOS/actions/runs/39140272964/artifacts/12141610694

## Missing real-world browser support
- Dynamic segmented store is not wired into \`TreeBuilder\`.
- Current parser still lacks complete full-document insertion modes, full
  SVG/MathML namespaces, and complicated adoption-agency/scripting cases.
- No full HTML Living Standard tokenizer or named-character-reference table.
- 44 demonstrated tree mismatches need targeted fixes; the first category
  includes nested buttons/anchors, SVG, unknown formatting and table contexts.
- No production DOM mutation/event API, CSS cascade, Flex/Grid, shaping,
  JavaScript-binding, paint/hit-test integration, or Web Platform Test score.
- No bootable EFI or Acer Nitro 5 hardware tests performed.

## Proposed H3C4: fail-closed dynamic DOM integration

1. Introduce a storage interface for DOM node creation, child/parent links
   and stable handles. Migrate the actual TreeBuilder to \`SegmentedNodeArena\`
   **on a separate branch**, preserving all existing regression tests.
2. Implement bounded failure propagation across all parser insertion paths.
   Test 2048+ nodes in a live document, not only the standalone arena.
3. Compare against the historical official \`#document\` expectations, not
   just Python's element/text output. Begin with adoption, block and table
   fixtures, then add current Web Platform Tests.
4. Keep the parser in Ring-3 and stage integration only against the exact
   authoritative full R95 M1-or-later source, preserving Wi-Fi/AX200,
   Ethernet, exFAT/FAT32, USB, SMP, Baofeng mesh, AI and kernel behavior.

Specifications:
https://html.spec.whatwg.org/multipage/parsing.html
https://github.com/web-platform-tests/wpt/tree/master/html/syntax/parsing
