# Trinity Navigator — HTML5 + CSS integrated development source R2

**Status: SOURCE INTEGRATION AND HOST QUALIFICATION ONLY.**
This package is the **entire tracked TrinityOS GitHub repository tree** at
the recorded integration commit, not a delta-only patch. It is NOT the
authoritative newer R95 source, not bootable EFI and not hardware-qualified.

## Scope / source lineage

1. GitHub \`main\` baseline: **Desktop Functionality R2, October 11, 2026**.
   See root \`SOURCE_SNAPSHOT.md\` for the exact original baseline.
2. HTML5 development H1–H3C5: branch
   \`prototype/html5-css-engine-h3c5\`. The compact segmented-only
   \`TreeBuilder\` passed real large-DOM and sanitizer host tests.
3. CSS Syntax C1: branch \`prototype/css-engine-c1\`.
4. GitHub pull request **#4** merged CSS C1 into isolated staging branch
   \`integration/navigator-html5-css-r1\` on October 18, 2026:
   https://github.com/jjcd-boop/TrinityOS/pull/4
5. THIS branch:
   \`integration/navigator-html5-css-c2-h4a-source-r2\` builds upon the
   above merged staging line and adds C2 selector/cascade + early H4A
   read-only DOM style evaluation.

## Implemented in this pass

**CSS C2 selectors and specificity**
- Bounded, stand-alone C++20 selector-list parser: type, universal, #id,
  .class, [attr], [attr=value], :root, child (>) and descendant combinators,
  comma-separated groups.
- Element matching traverses the *actual H3C5 HTML DOM* through stable,
  checked parent and sibling pointers.
- Unsupported CSS selectors (e.g. :hover, ::before, +, ~, complex
  attribute operators, :is(), :where(), :not(), :has()) are rejected or
  ignored, not misapplied as a simpler selector.
- Basic id/class/type specificity weights and source rule order.

**H4A computed-style integration, READ-ONLY**
- Real C1 Stylesheet rules applied to HTML Element nodes through a
  \`StyleResolver\`, returning a small immutable \`ComputedStyle\` result
  by value into caller-owned output. The DOM and source sheet are untouched.
- CSS important declarations win over non-important; among equal priority
  the more specific selector and later declaration win.
- Initial supported properties: color, background-color, display,
  font-size, font-weight, margin and padding.
- Basic inheritance: color, font-size, font-weight; defaults for
  transparent background and inline/block display.
- A **caller-owned reusable SelectorList scratch** is explicitly required
  by the resolver. This scratch is much larger than a small GUI stack:
  allocate it in Ring-3 service-owned memory, never on the Trinity
  kernel or compositor stack.

**Existing H3C5 HTML document storage**
- Build-time \`TRINITY_HTML5_SEGMENTED_ONLY\` mode eliminates the
  384-node inline fallback; separate legacy configuration remains for
  existing tests. Both modes retain bounded node storage.
- C++ ABI warning: do not link translation units built with inconsistent
  \`TRINITY_HTML5_SEGMENTED_ONLY\` modes.

## Verification

Automated GitHub Actions workflow:
\`.github/workflows/trinity-navigator-full-source-c2-r2.yml\`

The CI builds and tests:
- Freestanding-style Clang C++20 syntax with no exceptions or RTTI.
- C1 tokenizer/declaration parser tests with ASan/UBSan.
- C2 selector parsing, child/descendant matching, specificity,
  !important/order, inherited properties and unsupported selector
  rejection on actual HTML DOM nodes, with ASan/UBSan.
- H3C5 compact segmented live-document tests with ASan/UBSan.
- Earlier HTML H3C2/CSS flow regressions with ASan/UBSan.
- A verified \`git archive\` ZIP of the **entire tracked repository tree**
  plus SHA-256 checksum and scope/provenance text. The workflow uploads
  that ZIP as an artifact, named
  \`TrinityOS_NAVIGATOR_HTML5_CSS_C2_H4A_FULL_GITHUB_SOURCE_R2\`.

## Explicit omissions

**This is not a production Navigator GUI/browser merge.** Specifically:
- New HTML/CSS sources are under \`experiments/\`; the production browser's
  network fetcher, certificate validation, resource discovery, image raster
  decoder, painter and input event pipeline remain unchanged.
- No complete CSS cascade origins/layers/revert, inline \`style=\`,
  custom properties/var(), media/supports queries, pseudo-elements,
  CSSOM, Flexbox, Grid, fonts, or dynamic DOM binding yet.
- Current \`StyleResolver::compute()\` is a bounded correctness
  prototype and can be expensive on large pages because it parses
  selectors for each rule and ancestor. Before UI adoption, compile
  selectors once and cache computed styles by DOM generation.
- Modern HTML parsing remains only partially conformant. The earlier
  240-case historical survey showed 187 matches, 44 tree mismatches,
  2 rejections and 7 unsupported comparisons. This pass did not fix
  these parsing mismatches.
- No production EFI or physical Acer Nitro 5 test, no native browser
  repaint/pointer interaction verification.

## Safe integration with current authoritative Trinity R95

Do **not** overwrite a newer R95 full-source tree with the GitHub
October-11 baseline. To incorporate this development into current
production, take the *exact latest authoritative* R95 or later ZIP,
stage only the reviewed \`experiments/html5_css_h1\` and
\`experiments/css_engine_c1\` modules, then:
1. Add a Ring-3 capability-isolated parser/style-service link target.
2. Compile **every** TreeBuilder translation unit with the same
   \`TRINITY_HTML5_SEGMENTED_ONLY\` choice.
3. Give the HTML parser and CSS selector workspace caller-owned,
   quota-controlled memory; never put them on a small GUI stack.
4. Build a bounded read-only DOM/CSS IPC handoff first, not a
   wholesale replacement of Navigator's current DOM/layout engine.
5. Audit retention of shared geometry for painting, scrolling and hit
   testing; integrate network image/data fetching independently.
6. Run all source tests and then production EFI build and physical
   hardware tests, only then promote browser changes.
7. Leave kernel boot/SMP admission, Wi-Fi/AX200, Realtek/ICS,
   exFAT/FAT32, USB, AI/FunctionGemma and Baofeng radio untouched.

## Recommended next passes

**C3:** compile selector AST once, CSS inheritance/cascade including
inline style and custom properties, media/supports selectors.
**H4B:** retained read-only style/layout snapshots, paint/hit-test
rectangle consistency tests, and small production offscreen preview.
**Production:** merge only to *newest authoritative source* once attached
or made available as a complete working source tree.
