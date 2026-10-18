# Trinity Navigator — CSS C1 foundation

**Status:** independent C++20 CSS tokenizer / declaration parser, host-tested
under AddressSanitizer and UndefinedBehaviorSanitizer. It is NOT integrated
into Trinity Navigator, is NOT the complete CSS cascade, and has NOT been
included in any EFI.

Branch: \`prototype/css-engine-c1\`, based on the tested HTML H3C4 checkpoint.

## Files
- \`experiments/css_engine_c1/include/trinity_css_c1.hpp\`
- \`experiments/css_engine_c1/src/trinity_css_c1.cpp\`
- \`experiments/css_engine_c1/tests/css_c1_host.cpp\`
- \`.github/workflows/trinity-css-c1.yml\`

## Functionality
- Allocation-free CSS tokenization: identifiers, escaped identifiers,
  at-keywords, hashes, strings, bad strings, numbers, percentages,
  dimensions, functions, unquoted URLs, bad URLs, delimiters, punctuation
  and whitespace; CSS comments are skipped.
- Bounded stylesheet qualified rules with source-preserved selector
  preludes, declaration names, values and final \`!important\` flags.
- Declaration splitting accounts for strings, parentheses, brackets,
  nested braces and comments. E.g., semicolons inside URL/functions
  or strings do not split a declaration.
- At-rules, including nested \`@media\`, are skipped as complete units
  and **not yet interpreted**. They must not silently turn into top-level
  selector rules.
- Resource limits: 64 rules, 24 declarations per rule, 255 selector
  bytes, 63 property bytes, 511 value bytes, 255 token bytes, and
  bounded token scanning.
- Any overflow is reported via an explicit parser error. Truncation is
  never treated as a valid parsed declaration.

## Tests
GitHub Actions: https://github.com/jjcd-boop/TrinityOS/actions/runs/39142477854

Freestanding-style \`-std=c++20 -ffreestanding -fno-exceptions -fno-rtti
-Wall -Wextra -Werror\` syntax: PASS.
Clang host ASan + UBSan tests: PASS.
Cases cover escaped identifiers, Unicode escape decode, scientific
number dimensions, comments, malformed URLs, nested \`calc()\` values,
quoted semicolons, \`!important\`, skipped nested \`@media\`, malformed
declaration recovery and resource-limit failures.

## Known gaps
- **Not full CSS Syntax Level 3 tokenization** (CR escapes/normalization,
  all identifiers/URLs/serialization and input encoding edges remain).
- No selector AST, selector matching, specificity, inheritance,
  cascade, computed style, CSS variables, \`@media\` evaluation,
  \`@supports\`, \`@font-face\`, or CSSOM.
- No real style application to HTML nodes, box layout, Flexbox/Grid,
  font metrics, images, repainting or pointer-hit geometry.
- The current fixed array \`Stylesheet\` is large; it must be
  owned by an isolated Ring-3 style service and eventually be
  replaced by a segmented/quota-controlled design.
- No Trinity cross build, ELF, EFI or physical hardware pass.

## Next phase: C2 plus H4A
1. Implement selector parsing and CSS specificity in testable C++.
2. Implement cascade origin/order/\`!important\` and property inheritance.
3. Resolve style rules onto the H3C5 DOM document tree through a read-only
   adapter; preserve layout and input hit-testing invariants.
4. Retain separate CSS, HTML, Wi-Fi, filesystem, USB, radio and AI branches
   until checked against the exact authoritative Trinity source.

Normative reference (CSSWG Candidate Recommendation Draft 1 October 2026):
https://www.w3.org/TR/css-syntax-3/
