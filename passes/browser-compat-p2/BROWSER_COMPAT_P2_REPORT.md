# Trinity Browser Compatibility P2

Date: 2026-10-10
Baseline: Browser Compatibility P1 on VM Pass 3C-R2 Production Freeze

## Scope

P2 upgrades the shared HTTP compatibility layer without expanding arbitrary page-code capabilities.

Implemented:

- HTTP `Accept-Encoding: gzip, deflate` negotiation for documents and ordinary resources.
- Media/range fetches remain `Accept-Encoding: identity` so byte ranges continue to address the real media container.
- Ring-3 gzip decoder with RFC1951 DEFLATE, gzip CRC32/ISIZE validation, and bounded output.
- Ring-3 `deflate` decoder supporting both RFC1950/zlib wrapping with Adler-32 validation and legacy raw RFC1951 payloads.
- Explicit rejection of unsupported content encodings instead of interpreting compressed bytes as HTML/CSS/JS/images.
- Response metadata for `ETag`, `Last-Modified`, and `Cache-Control`.
- Conditional GET support using `If-None-Match` and `If-Modified-Since` through the bounded Browser syscall contract.
- Three-slot, 64 KiB-per-entry decoded entity cache in Ring-3.
- `304 Not Modified` entity restoration with cached content type.
- `Cache-Control: no-store` prevents entity insertion.
- POST, range, and media requests do not receive cache validators.

## Trust boundary

Compression and entity caching remain in the Browser's Ring-3 address space. Ring-0 transports bounded header values and response bytes but does not parse compressed page content. Request validator values are CR/LF checked by the same header-sanitization path introduced in P1.

## Verification

The following modified translation units compile with Trinity's production freestanding Windows target flags and `-Wall -Wextra -Werror`:

- `src/kernel/netstack.cpp`
- `src/kernel/network.cpp`
- `src/kernel/syscall.cpp`
- `src/user/web_runtime.cpp`
- `src/user/apps/browser.cpp`

Regression/contract verification:

- Browser P1 static contract: PASS (10 checks)
- Browser P2 static contract: PASS (10 checks)
- Browser P2 decoder test: PASS
  - gzip
  - RFC1950/zlib deflate
  - raw DEFLATE fallback
  - unsupported encoding rejection
  - decompressed-output safety limit

## Remaining compatibility limits

P2 materially improves ordinary website transport compatibility, especially servers that default to compressed HTML/CSS/JavaScript, but does not make Trinity a modern general-purpose browser by itself. Major remaining work includes Brotli, richer TLS interoperability, a real DOM, modern CSS/layout, a real ECMAScript runtime/event loop, Fetch/XHR/browser APIs, and production media demux/codec/playback for YouTube.

Brotli is intentionally deferred to the next transport pass rather than advertising `br` before a decoder is present.
