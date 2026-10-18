# Trinity Navigator H1 — HTML5 / CSS modernization groundwork

**Status:** isolated C++20 experiment; not merged into the R95 Modernization M1 production source or EFI.

## Why
Trinity has an existing partial DOM, custom HTML tokenization and mixed paint/layout
paths. Website rendering requires a standards-oriented tokenizer, an HTML tree builder,
a proper CSS cascade and retained layout whose geometry is shared by painting and input.

## Included here
- Bounded HTML tokenizer with token types, normalized tag/attribute names, quoted,
  unquoted and boolean attributes, first-duplicate-wins, comments, doctypes,
  UTF-8 character references and simplified script/style raw text and RCDATA modes.
- Bounded static-block CSS geometry with nested width calculation, padding, margins,
  border-box semantics, vertical flow and basic inline wrapping.
- Host regression tests and freestanding-style compile checks.
- No third-party browser source copied; WHATWG HTML parsing and CSS specs are
  design references.

## Known standards gaps (do not report as HTML5-compliant)
- **Tokenizer:** incomplete states for script escapes/double escapes; incomplete
  named-entity table and semicolonless context rules; no streaming chunk continuation,
  null replacement, SVG/MathML special cases or parse error reporting.
- **Tree construction:** not implemented. Requires insertion modes, implied elements,
  adoption agency algorithm, foster parenting, templates, foreign-content nodes,
  fragment parsing and document.write interactions.
- **CSS:** current geometry is not a CSS parser or cascade. No inline formatting with
  shaped text, margin collapsing, float placement, Flex/Grid, positioning or media queries.
- **Integration:** production R95 M1 archive is the authoritative source but is
  not checked in as a full working tree on the current GitHub main branch.
  Avoid applying these prototype files as if the production renderer is integrated.

## Integration conditions
1. Import this component into an **exact** source checkpoint newer than or equal to
   R95 M1, preserving all independently staged USB, mesh, Wi-Fi, storage and console work.
2. Replace unstructured HTML scanning incrementally, making tokenization and DOM
   tree-building separately testable.
3. Wire retained layout rects through all paint and hit-testing consumers.
4. Run html5lib tokenizer cases and the web-platform-tests HTML parsing/CSS suites.
5. Build Trinity freestanding Ring-3 ELF and EFI. Test malformed HTML and limits.
6. Hardware-test actual webpages and compare screenshots/click geometry with Chrome.

## Build on a host
\`\`\`sh
clang++ -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Iexperiments/html5_css_h1/include \
  experiments/html5_css_h1/src/trinity_html5_tokenizer.cpp \
  experiments/html5_css_h1/tests/html_tokenizer_host.cpp \
  -o /tmp/trinity_html_h1
/tmp/trinity_html_h1

clang++ -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Iexperiments/html5_css_h1/include \
  experiments/html5_css_h1/src/trinity_css_flow.cpp \
  experiments/html5_css_h1/tests/css_flow_host.cpp \
  -o /tmp/trinity_css_h1
/tmp/trinity_css_h1
\`\`\`

Design references:
- https://html.spec.whatwg.org/multipage/parsing.html
- https://www.w3.org/TR/css-display-3/
- https://web-platform-tests.org/
