# Trinity Navigator H2 — bounded HTML tree construction

Status: **prototype host tests and freestanding-style syntax passed** using
Clang 18, \`-Wall -Wextra -Werror -fno-exceptions -fno-rtti\` and
ASan/UBSan. This is a tested GitHub development branch, **not** a new EFI.

## Branch and test
- Branch: \`prototype/html5-css-engine-h2\`
- CI: https://github.com/jjcd-boop/TrinityOS/actions/runs/39134229006
- Foundation: H1 tokenizer and CSS flow component in \`experiments/html5_css_h1\`
- New: \`include/trinity_html5_tree.hpp\`, \`src/trinity_html5_tree.cpp\`
- New: \`tests/html_tree_host.cpp\`
- GitHub Actions: \`.github/workflows/trinity-html5-css-h2.yml\`

## Implemented
1. \`Document\`, \`html\`, \`head\`, and \`body\` nodes, including implied skeleton.
2. Element nodes, sibling/parent/child links, text, comments and doctypes.
3. Fixed-capacity per-node attribute storage; first duplicate wins via H1.
4. Head content including \`title\`, \`style\` and \`script\` through H1's
   early raw-text/RCDATA tokenizer support.
5. Simplified in-body repair for commonly omitted closing \`p\`, \`li\`,
   \`dt\`, \`dd\` and headings.
6. Simplified HTML table repair: implied \`tbody\` and, where appropriate,
   an implied \`tr\`, plus closing cells/rows.
7. Correct HTML self-closing slash behavior for ordinary HTML non-void tags.
8. Bounded document arena (384 nodes), open-element stack (64), token count,
   attribute count, and preservation of long text by splitting nodes.
9. Fail-closed errors on limits rather than silently truncating node content.

## Host tests
- H1 tokenizer regression: pass.
- H1 CSS flow regression: pass.
- H2 node structure, head text, optional end tags, list/table construction,
  void elements, stray body \`</p>\`, attributes, long text, empty documents,
  and limit rejection: pass.
- Production Trinity x86-64 image/EFI: **not built** in this branch.
- Actual Acer Nitro 5 hardware: **not tested**.

## Important limits
This is an initial HTML tree builder, **not the full WHATWG HTML Living Standard**.
Missing essential pieces include the adoption agency algorithm; active formatting
elements; full table foster parenting; table insertion modes; template parsing;
SVG/MathML namespaces; fragment parsing; \`document.write\` and parser reentrancy;
all tokenizer states and named character references; encoding sniffing and
recovery; DOM mutation/events; and a full CSS cascade/layout connection.

The C++ tree builder is also **not wired into Trinity Navigator's production
renderer**. It is a separate source checkpoint. Do not overwrite unrelated
filesystem, networking, USB, SMP, kernel or amateur-radio source with it.

## Next recommended passes
H3: Implement full tokenizer and tree-construction conformance with html5lib
fixtures and targeted Web Platform Tests. Split adoption-agency, table
foster-parenting and template cases into separately testable changes.

H4: Connect a retained DOM representation to CSS style resolution and layout,
while keeping painting and click-hit geometry sourced from the same boxes.

H5: Integrate on a known-good complete Trinity R95 M1-or-later source tree,
rebuild bootable EFI, and request user hardware qualification.

HTML parsing design reference:
https://html.spec.whatwg.org/multipage/parsing.html

No code from Ladybird, Chromium, WebKit or Servo has been imported.
