# Trinity Navigator — HTML5 H3a: Table Foster Parenting / Nested Table Isolation

**Status:** Original C++ source in experimental GitHub branch; freestanding-style compilation and ASan/UBSan host tests passed. **NOT integrated into production R95 M1 EFI**.

## Source

Branch: `prototype/html5-css-engine-h3a`

New/updated files:
- `experiments/html5_css_h1/include/trinity_html5_tree.hpp`
- `experiments/html5_css_h1/src/trinity_html5_tree.cpp`
- `experiments/html5_css_h1/tests/html_table_foster_host.cpp`
- `.github/workflows/trinity-html5-css-h3a.yml`

## Implementation

- Generalized append-only document nodes to support **bounded insertion before an existing sibling**.
- Added sibling predecessor lookup and parent checks, preserving first/last sibling links.
- Implemented initial HTML **foster parenting** for non-whitespace text and non-table elements encountered directly inside table scaffolding (`table`, `tbody`, `thead`, `tfoot`, `tr`).
- Preserved normal positioning for table rows, cells, captions, structure elements and text *inside* cells.
- Fixed a previously uncovered nested-table bug: automatic closing of `tr`, `td`, `th` and matching structural end tags now stops at the **innermost table boundary** rather than reaching an outer table.
- Reused original H1 tokenizer and H2 tree builder; no third-party browser code imported.

## Verification

GitHub Actions run (before this report, same source):
https://github.com/jjcd-boop/TrinityOS/actions/runs/39135028416

- Freestanding C++ syntax with `-Wall -Wextra -Werror -fno-exceptions -fno-rtti`: PASS.
- H1 tokenizer regression, ASan/UBSan: PASS.
- H1 CSS flow regression, ASan/UBSan: PASS.
- H2 DOM tree builder regression, ASan/UBSan: PASS.
- H3a foster parenting and DOM link integrity, ASan/UBSan: PASS.
- **300 deterministic malformed-markup stress cases**, with graph invariants checked on successful parses: PASS.

Tests cover foster text and markup ordering, sibling insertion, nested tables, table whitespace, caption/cell handling, long text split across nodes, and malformed table fragments.

## Explicit limitations

This **does not complete H3 or full HTML5**. Current tree construction is still a simplified model and does not implement all WHATWG insertion modes, foster parenting across all table-mode transitions, adoption agency algorithm, active formatting elements, template parsing, SVG/MathML namespaces, script-aware tree construction, or parser reentrancy.

H1 tokenizer still lacks full named-character-reference coverage and multiple HTML tokenizer states. The DOM tree is not integrated into the running Navigator or CSS painter.

## Integration requirements

1. Start from an **exact complete R95 M1-or-later source package**, not a stale GitHub main working tree.
2. Add the experimental files as a **separate Ring-3 HTML parsing service** with bounded memory allocation. The fixed node arena must not go on a small application stack or into the kernel.
3. Connect parser results to Trinity's real DOM nodes via an audited adapter before letting it replace existing webpage parsing.
4. Keep the GUI paint rectangles and pointer hit rectangles sourced from the same retained layout geometry.
5. Run html5lib tree-construction fixtures and selected WPT HTML parser tests; identify remaining mismatches rather than marking the browser standards-compliant.
6. Build/verify the Trinity EFI and hardware-test on the Acer Nitro 5.

Do not merge or overwrite unrelated Wi-Fi, Ethernet, USB, storage, SMP, radio, kernel, or filesystem work.

## Next phase

**H3b:** Implement and test the HTML active-formatting-elements list and adoption agency algorithm in bounded, audited passes, using html5lib fixtures and rendering differential tests. Then cover remaining table insertion modes and integrate with DOM/CSS.

Design reference: https://html.spec.whatwg.org/multipage/parsing.html
