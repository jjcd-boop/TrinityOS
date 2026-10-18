# Trinity Navigator — H3B1 Bounded HTML Active Formatting + Adoption Repair

**Status:** Independent C++20 prototype qualified on the Ubuntu GitHub Actions host
with Clang freestanding-style syntax and ASan/UBSan regressions. **NOT merged
with or hardware-qualified for the production R95 M1 Trinity EFI.**

## Branch
\`prototype/html5-css-engine-h3b\`
https://github.com/jjcd-boop/TrinityOS/tree/prototype/html5-css-engine-h3b/experiments/html5_css_h1

## Scope
Based directly on H3A (table foster parenting) without importing code from
Ladybird, WebKit, Chromium, Servo or another browser engine.

Updated:
- \`include/trinity_html5_tree.hpp\`
- \`src/trinity_html5_tree.cpp\`

Added:
- \`tests/html_formatting_host.cpp\`
- \`.github/workflows/trinity-html5-css-h3b.yml\`

## Implemented
1. Bounded active-formatting list separate from open-element stack, with
   table/caption/cell markers and explicit capacity errors.
2. Reconstruction of formatting elements after mismatched end tags, including
   copy of original element attributes.
3. Simplified Noah's Ark clause: limit to three identical active-formatting
   signatures between markers.
4. Repair of nested \`<a>\` elements by ending the previous active link before
   starting another.
5. Adoption-agency no-furthest-block branch (e.g. \`<b><i>1</b>2</i>\`).
6. Direct-child furthest-block repair (e.g. \`<b>1<p>2</b>3</p>\`), including
   checked DOM reparenting, clone of the formatting element, bounded child moves,
   and open-stack reconciliation.
7. Preservation of H3A's nested-table boundaries and foster parenting.
8. Deterministic malformed-markup tests with parent/child/sibling acyclicity
   and ownership invariants.

## Qualification
Successful Github Actions verification before report commit:
https://github.com/jjcd-boop/TrinityOS/actions/runs/39135824349

- Freestanding-style C++20 syntax (-Wall -Wextra -Werror -fno-exceptions -fno-rtti): pass.
- H1 tokenizer, CSS flow, H2 DOM, H3A table foster regressions: pass.
- H3B1 formatting/adoption cases with ASan/UBSan: pass.
- 400 deterministic malformed-markup samples, checking either coherent
  document graphs or explicit bounds failure: pass.

## Critical standards and runtime limitations
This is **not** the full WHATWG adoption-agency algorithm. It does not yet
perform the general multi-level furthest-block inner loop, bookmark movement,
full scope checking, all special tags, or every formatting marker interaction.
Complex cases currently use a bounded fallback, and they can still produce
different DOM trees from Chrome/Firefox.

Further, the H1 tokenizer is a reduced implementation of the HTML Living
Standard, and this branch has not been connected to Trinity's production
browser DOM, layout, JS APIs, or paint/hit-test geometry. This source has not
been built into a Trinity freestanding ELF or tested on Trinity hardware.

## Integration constraints
- Take the **exact authoritative R95 M1 or later full source** as input, not
  the old GitHub default branch.
- Introduce the parser as an isolated Ring-3 service with a strict memory
  budget and stable C ABI / IPC boundaries. The node arena must not live in
  small GUI stacks or Ring-0.
- Preserve filesystem, Ethernet/Wi-Fi, USB, SMP, Baofeng mesh, AI, and
  kernel bring-up code unchanged.
- Expand html5lib / Web Platform Tests and differential DOM tests before
  enabling the new engine for normal browsing.
- Qualify production builds and hardware separately.

## Next checkpoint (H3B2)
Complete remaining adoption-agency steps including bookmarks, scope and
intermediate formatting nodes; use html5lib tree-construction expectations
and fuzz verification to prevent code generation shortcuts from silently
passing unit fixtures.

Standards design reference:
https://html.spec.whatwg.org/multipage/parsing.html#the-adoption-agency-algorithm
