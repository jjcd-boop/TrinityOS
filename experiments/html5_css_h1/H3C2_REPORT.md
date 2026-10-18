# Trinity Navigator — H3C2 HTML Table Text, Caption and Column-Group Modes

**Status:** Verified independent C++ development checkpoint. This is **not**
integrated into the authoritative R95 M1-or-later Trinity Navigator build.
No Trinity EFI/ELF or Acer Nitro 5 hardware pass was performed.

## Authoritative experimental source
Branch: \`prototype/html5-css-engine-h3c2\`
Parent: \`prototype/html5-css-engine-h3c1\`

Changed files:
- \`experiments/html5_css_h1/include/trinity_html5_tree.hpp\`
- \`experiments/html5_css_h1/src/trinity_html5_tree.cpp\`
- \`experiments/html5_css_h1/tests/html_tree_dump_host.cpp\`
- \`experiments/html5_css_h1/tests/html5lib_differential_h3b2.py\`

Added files:
- \`experiments/html5_css_h1/tests/html_table_modes_h3c2_host.cpp\`
- \`.github/workflows/trinity-html5-css-h3c2.yml\`
- \`experiments/html5_css_h1/H3C2_REPORT.md\`

## Implemented
1. Bounded **pending table character-token** queue (8192 bytes). Consecutive
   character tokens are classified as one run, even when the H1 tokenizer
   splits the original text across multiple tokens.
2. All-whitespace table-text runs remain in the current table scaffold.
   Mixed runs (including leading/trailing whitespace) are reprocessed as
   foster-parented text, after restoring active formatting where required.
3. Pending text flushes before non-character tokens, comments, doctypes,
   and EOF. NUL tokens are dropped per this table-text rule. Overflow
   fails with \`TokenOverflow\` instead of truncating characters.
4. Caption scope closing on new table structure tags and on \`</table>\`.
   Clear the active-formatting marker when leaving a caption.
5. Implied \`colgroup\` for \`<col>\` directly under an open table, with
   explicit \`colgroup\` end-tag support.
6. Preserved the H3C1 innermost-table boundary, tbody/tr/td context
   clearing, adoption-agency repairs and foster-parent sibling placement.
7. Fixed the **test oracle**, not the runtime parser, to escape multiline
   text consistently and exclude HTML comment nodes from an explicitly
   element/text-only DOM comparison.

## Verification

GitHub Actions on the \`prototype/html5-css-engine-h3c2\` branch:
- Freestanding-style Clang C++20 syntax with \`-Wall -Wextra -Werror
  -ffreestanding -fno-exceptions -fno-rtti\`: PASS.
- H1 tokenizer / CSS geometry tests: PASS.
- H2 document tree, H3A table foster, H3B1/H3B2 formatting, H3C1
  table-scope tests under ASan + UBSan: PASS.
- New H3C2 table text/caption/colgroup/rowgroup tests under ASan + UBSan: PASS.
- **320 additional deterministic malformed-markup samples** with DOM graph
  invariants for successful parses and explicit errors for rejected parses:
  PASS.
- **35 of 35 targeted \`html5lib\` element/text tree-shape comparisons match**.
  All 35 are now required, fail-on-mismatch CI checks, up from 23 in H3C1.

### Test interpretation
The 35 examples cover selected tree shapes only; they do not validate all
WHATWG parsing rules, attributes, namespaces, comments, encodings, script
behavior, DOM APIs or painted output. The reference comparison uses Python
\`html5lib\` **only in host CI**; no Python runtime or reference parser is
shipped inside Trinity OS.

## Important missing behaviors
- Full HTML insertion-mode state machine (in table/in table text/in caption/
  in column group/in table body/in row/in cell, template, etc.) with every
  standards-defined edge case.
- Full tokenizer script escape states, character reference names, encoding
  handling, SVG/MathML and template content, fragment parsing and reentrancy.
- Comprehensive html5lib-tests / Web Platform Tests and property fuzzing.
- Dynamic quota-enforced DOM: current arena is fixed at 384 nodes.
  A parser with 384 nodes cannot render the real modern web.
- Real CSS cascade/Flexbox/Grid, font shaping, DOM mutation, QuickJS bridge,
  painting, input hit-testing, and integration into the existing browser.
- Any actual Trinity EFI test or hardware qualification.

## Integration restrictions
The authoritative production source remains the exact R95 M1-or-later
Trinity tree. Do not blindly merge these files onto an older GitHub snapshot.
First audit their ABI and memory footprint, then integrate through an
isolated Ring-3 parser service with a bounded request/response interface,
reusing the same layout rectangles for painting and clicking.

Do not change USB, storage, FAT32/exFAT, Realtek, AX200, CPU admission,
Baofeng mesh, AI, or kernel code as part of parser integration.

## Next phase
H3C3: broader html5lib-test corpus and table-mode correctness, with an
explicit checklist of incomplete insertion modes and reference mismatches.
After that, design a segmented, quota-enforced document arena and connect
DOM construction to a retained CSS layout system before EFI qualification.

Spec: https://html.spec.whatwg.org/multipage/parsing.html
