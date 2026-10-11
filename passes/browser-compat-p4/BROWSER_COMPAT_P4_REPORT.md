# Trinity OS Browser Compatibility P4 — Bounded DOM Foundation

Date: 2026-10-10
Baseline: Browser Compatibility P3B / VM Pass 3C-R2 production freeze

## Goal

Move Trinity's Browser away from repeated raw-string HTML scanning toward a bounded document tree that can support modern layout, JavaScript DOM bindings, forms, CSS matching, and media elements without introducing heap ownership into the current Browser process.

## Implemented

- Fixed-capacity Ring-3 `DomDocument` with 768 nodes and 1024 attributes.
- Zero-copy node/attribute slices reference the immutable page buffer instead of duplicating HTML strings.
- Document, element, and text node types.
- Parent/first-child/next-sibling relationships.
- Case-insensitive HTML element and attribute lookup helpers.
- Quoted, unquoted, and boolean attribute parsing.
- HTML comments, declarations, processing-instruction style records, start tags, end tags, self-closing tags, and standard void elements.
- Raw-text handling for `<script>` and `<style>` so markup-like characters inside those elements are not reparsed as document nodes.
- Bounded nesting stack with malformed/overflow telemetry instead of unbounded recursion.
- DOM text-content extraction with HTML entity decoding.
- Browser integration: remote and local HTML pages now build the DOM on load.
- Browser title extraction consumes the DOM first, retaining the legacy raw scanner only as a compatibility fallback.
- The DOM is stored in static Browser-owned Ring-3 memory rather than expanding the BrowserState stack object.

## Verification

- `web_runtime.cpp`: strict freestanding compile with `-Wall -Wextra -Werror`: PASS.
- `browser.cpp`: strict freestanding compile with `-Wall -Wextra -Werror`: PASS.
- P4 runtime DOM test: PASS (10/10).
- P4 static integration contract: PASS (12/12).
- P1 static regression: PASS (10/10).
- P2 static regression: PASS (10/10).
- P3A static regression: PASS (10/10).

## Brotli status

`br` remains intentionally unadvertised. The execution environment could not import the upstream Brotli source archive into the workspace, and the available hosted static library has hosted ABI dependencies (`malloc`, stack-protector/glibc symbols) that are not acceptable for Trinity's freestanding Ring-3 runtime. Trinity therefore continues to advertise only `gzip, deflate` rather than claiming unsafe or incomplete Brotli support.

A future Brotli pass should vendor the official decoder source and drive it through a fixed custom allocator arena.

## Architectural significance

This is the first step where Browser compatibility advances the actual browser engine rather than just transport. The DOM tree becomes the common substrate for the next major stages:

1. CSS selector matching/cascade against document nodes.
2. Retained layout boxes instead of reparsing raw HTML every render pass.
3. DOM mutation APIs and event targets.
4. JavaScript bindings for `document`, elements, attributes, navigation, forms, and events.
5. Media element integration for `<video>` / `<audio>`.

## Known limitations

This is a bounded HTML parser foundation, not a complete WHATWG HTML tree-construction implementation. Foster parenting, template insertion modes, foreign-content namespaces, full error recovery, custom elements, shadow DOM, and mutation are not yet implemented.
