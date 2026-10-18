# Trinity Navigator — HTML5 H3B2 Adoption-Agency Repair

Status: **experimental source checkpoint** (C++20); host compilation and tests
pass. **Not merged into production R95 M1, no Trinity ELF/EFI or hardware test.**

## Branch and files
- GitHub branch: \`prototype/html5-css-engine-h3b2\`.
- Base: H3B1 development branch.
- Implementation: \`include/trinity_html5_tree.hpp\`,
  \`src/trinity_html5_tree.cpp\`.
- Structural stress tests: \`tests/html_adoption_h3b2_host.cpp\`.
- Independent oracle: \`tests/html_tree_dump_host.cpp\` and
  \`tests/html5lib_differential_h3b2.py\`.
- CI: \`.github/workflows/trinity-html5-css-h3b2.yml\`.

## New functionality

1. Bounded active-formatting list insertions and per-node lookup.
2. Bounded stack removal and insertion preserving stack order.
3. A limited formatting-in-scope check with table/cell/template boundaries.
4. Intermediate-node handling: removes non-active nodes from the stack;
   clones active formatting elements, updates both stack and active-list
   entries, and reparents the last node.
5. Tracks the active-formatting bookmark through the inner adoption loop.
6. Creates the replacement formatting element, moves children into it,
   and inserts its entry immediately below the furthest block.
7. Repeats the outer adoption process at most eight times, with explicit
   resource limits and DOM ancestry checks on reparenting.
8. Preserves tests from H1 tokenizer, H1 CSS, H2 tree construction, H3A table
   foster parenting and H3B1 simplified formatting repairs.

## Verification

- Clang 18 freestanding-style C++ syntax \`-Wall -Wextra -Werror
  -ffreestanding -fno-exceptions -fno-rtti\`: PASS.
- H1 through H3B1 existing native sanitizer regressions: PASS.
- H3B2 tree invariants and 600 deterministic malformed-document
  samples (ASan + UBSan): PASS.
- Independent **html5lib 1.1** host-only reference check:
  **21 matching cases of 23**. The 21 matches are blocking CI assertions;
  the two known mismatches are tracked as exploratory cases.

Known reference mismatches (do NOT count as passing HTML5):
1. \`<table><b><tr><td>A</td></tr>B</table>C\`
   — incomplete \`in table\` insertion modes/foster parenting and
   misplaced active formatting during table structure creation.
2. \`<table><tr><td><b>A<table><tr><td>B</b>C</td></tr></table></td></tr></table>\`
   — nested-table cell/formatting scope and text-node merging.

The differential test compares element/text tree shapes, NOT the full set
of attributes, namespaces, DOM API semantics, or rendering output.
\`html5lib\` is used only as a Python reference in GitHub Actions: none of
its code ships with Trinity.

## Important restrictions

- Not the entire WHATWG HTML Living Standard: adoption scope handling,
  parser insertion modes, foreign content, templates, script reentrancy,
  tokenizer completeness, character references, and many algorithms remain.
- Current fixed node arena is 384 nodes; not suitable for unrestricted
  real webpages without a bounded segmented allocation strategy.
- No CSS cascade, modern Flex/Grid engine, JavaScript DOM binding, or
  browser renderer connection has been added by H3B2.
- No new EFI was built, and no Acer Nitro 5 hardware qualification exists.
- This independent source branch does not modify USB, exFAT/FAT32,
  AX200, Realtek, multicore, Baofeng, AI, bootloader, or kernel code.

## Recommended next step

**H3C1: implement genuine table insertion modes and their context-scoped
stack repairs**, targeting the two recorded html5lib mismatches. Then
expand html5lib / Web Platform Tests and progress toward the tokenizer's
remaining normative states.

**Integration:** stage against the exact authoritative R95 M1-or-later full
source, not GitHub main, using a dedicated Ring-3 parser service and
an audited adapter for Navigator DOM/render geometry. Run the EFI build
and hardware tests only after integrating source and checking dependencies.

Reference: https://html.spec.whatwg.org/multipage/parsing.html#the-adoption-agency-algorithm
No third-party browser implementation source was imported.
