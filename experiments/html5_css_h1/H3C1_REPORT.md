# Trinity Navigator — H3C1 Table Insertion-Context and Formatting Scope

**Checkpoint:** source committed to \`prototype/html5-css-engine-h3c1\`.
**Qualification:** host C++ tests pass with sanitizers; **not** integrated into Trinity Navigator R95 M1 production source, no EFI build or hardware test.

## Source

Base: \`prototype/html5-css-engine-h3b2\`.
Changes:
- \`experiments/html5_css_h1/src/trinity_html5_tree.cpp\`
- \`experiments/html5_css_h1/tests/html5lib_differential_h3b2.py\`
- \`experiments/html5_css_h1/tests/html_table_context_h3c1_host.cpp\`
- \`.github/workflows/trinity-html5-css-h3c1.yml\`
- This report.

No other Trinity OS subsystems or production source were changed.

## Changes

1. Implemented explicit **innermost-table stack boundaries** for row-group,
   row, and cell insertion, as a bounded subset of the WHATWG "clear stack
   back to table context / table body context / table row context" algorithms.
   Misnested formatting elements above a table no longer become parents of
   implied \`tbody\` and \`tr\` elements.
2. Non-whitespace table text now reconstructs active formatting elements
   before deciding the adjusted/foster insertion location, so misplaced
   \`<b>\` text is attached to the reconstructed element rather than becoming
   an unrelated text sibling.
3. When an end formatting tag cannot be found in the active formatting
   list after the latest cell/caption marker, the fallback now stops at a
   special element/table boundary. It must not close across a nested
   \`<table>\` or \`<td>\`.
4. The two H3B2 known-mismatch fixtures are promoted from exploratory
   tests to mandatory \`html5lib\` comparison assertions.
5. Added a standalone host test verifying table DOM structure, nested
   tables, formatting across contexts, valid table groups and preservation
   of parent/child/sibling link invariants under **400 deterministic
   malformed-markup samples**.

## Verification

GitHub Actions:
https://github.com/jjcd-boop/TrinityOS/actions/runs/39138142298

- Freestanding-style Clang C++20 syntax
  \`-ffreestanding -fno-exceptions -fno-rtti -Wall -Wextra -Werror\`: PASS.
- Existing H1 tokenizer and basic CSS tests: PASS.
- Existing H2, H3A, H3B1 and H3B2 HTML tree tests under ASan/UBSan: PASS.
- Independent \`html5lib 1.1\` tree-shape comparison:
  **23/23 mandatory samples matched** (H3B2: 21/23).
- H3C1 nested-table and context scope tests (ASan/UBSan): PASS.
- 400 new deterministic malformed-table samples (accepted trees have
  validated DOM graph invariants, rejected trees have explicit errors): PASS.

## Critical interpretation

**23/23 is only a small, targeted set of HTML tree-structure examples.**
It does not establish 100% HTML5 compliance or correct results on all
malformed real-world websites. The differential tree dumper compares
element and text shape, not all attributes, comments, DOM behavior,
JS execution, namespaces or CSS rendering.

The simplified parser is missing important WHATWG normative behaviors:
- Distinct complete table insertion modes and pending table character-token
  groups ("in table text", "in table", "in caption", "in table body",
  "in row", "in cell", "in column group").
- Complete adoption-agency algorithm, full active-formatting scope tables,
  template handling, SVG/MathML namespaces, fragment parsing, encoding sniffing
  and streaming/pause-reentrant tokenizer.
- Full HTML named character reference table, script escaped states,
  DOM mutations/events, CSS cascade/Flexbox/Grid and font shaping.

The bounded TreeBuilder arena is currently 384 nodes and cannot be used
as a production general-web document tree without redesigning its
service-owned, quota-enforced storage. It must not move into Ring-0 or
be allocated on the GUI stack.

## Next phase

H3C2: separate real HTML5 table insertion modes, pending table-character
token buffering and in-cell/caption context transitions. Expand
\`html5lib\` corpus using reference fixtures from
https://github.com/html5lib/html5lib-tests/tree/master/tree-construction
and Web Platform Tests before connecting the parser to Trinity Navigator's
real DOM painter and hit-testing.

Integrate only against the exact authoritative full Trinity R95 M1-or-later
source once available, preserving filesystem, USB, Ethernet/AX200, SMP,
Baofeng mesh, AI, and bootloader code. No EFI is released in this pass.

Standards reference: https://html.spec.whatwg.org/multipage/parsing.html#the-in-table-insertion-mode
