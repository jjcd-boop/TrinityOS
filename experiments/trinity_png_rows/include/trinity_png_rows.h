#ifndef TRINITY_RUST_PNG_ROWS_H
#define TRINITY_RUST_PNG_ROWS_H

#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Experimental Rust no_std PNG row-reconstruction ABI.
 * Input is already inflated RGB8 or RGBA8 PNG rows, each prefixed by a filter byte.
 * All three byte buffers must be valid and non-overlapping for the whole call.
 * Caller is responsible for PNG chunk/CRC validation and bounded DEFLATE decoding.
 *
 * Returns: 0 success; -1 pointer; -2 dimensions; -3 overflow;
 * -4 input length; -5 PNG filter; -6 small buffer; -7 overlapping buffers.
 */
int32_t trinity_png_rows_to_rgba(
    const uint8_t* inflated, size_t inflated_len,
    uint32_t width, uint32_t height, uint32_t channels,
    uint8_t* rgba_out, size_t rgba_out_len,
    uint32_t out_width, uint32_t out_height,
    uint8_t* scratch, size_t scratch_len);
#ifdef __cplusplus
}
#endif
#endif
