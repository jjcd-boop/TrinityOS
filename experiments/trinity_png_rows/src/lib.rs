#![no_std]

// Trinity OS exploratory component: PNG scanline reconstruction/downsampling.
// This does NOT inflate DEFLATE data, parse PNG chunks, or own disk/network access.
// ABI is intentionally plain C and contains no allocation.
//
// Safety contract: All pointers must refer to valid, non-overlapping regions of
// the declared lengths. The function checks arithmetic, bounds and overlap;
// it cannot validate that arbitrary foreign pointers are mapped.
use core::mem;

const MAX_SOURCE_DIM: usize = 8192;
const MAX_SOURCE_PIXELS: usize = 33_554_432;
const MAX_OUTPUT_DIM: usize = 2048;
const MAX_OUTPUT_PIXELS: usize = 4_194_304;
const MAX_INFLATED_BYTES: usize = 134_217_728;

const BAD_ARGUMENT: i32 = -1;
const BAD_DIMENSIONS: i32 = -2;
const OVERFLOW: i32 = -3;
const BAD_DATA_LENGTH: i32 = -4;
const BAD_FILTER: i32 = -5;
const BUFFER_TOO_SMALL: i32 = -6;
const ALIASING_REGIONS: i32 = -7;

fn ranges_overlap(a: usize, an: usize, b: usize, bn: usize) -> Result<bool, i32> {
    let a_end = a.checked_add(an).ok_or(OVERFLOW)?;
    let b_end = b.checked_add(bn).ok_or(OVERFLOW)?;
    Ok(a < b_end && b < a_end)
}

fn paeth(a: u8, b: u8, c: u8) -> u8 {
    let ai = a as i32;
    let bi = b as i32;
    let ci = c as i32;
    let p = ai + bi - ci;
    let da = (p - ai).abs();
    let db = (p - bi).abs();
    let dc = (p - ci).abs();
    if da <= db && da <= dc { a }
    else if db <= dc { b }
    else { c }
}

fn checked_sizes(
    width: usize,
    height: usize,
    channels: usize,
    ow: usize,
    oh: usize,
) -> Result<(usize, usize, usize, usize), i32> {
    if width == 0 || height == 0 || width > MAX_SOURCE_DIM || height > MAX_SOURCE_DIM
        || width.checked_mul(height).ok_or(OVERFLOW)? > MAX_SOURCE_PIXELS
        || (channels != 3 && channels != 4)
        || ow == 0 || oh == 0 || ow > MAX_OUTPUT_DIM || oh > MAX_OUTPUT_DIM
        || ow.checked_mul(oh).ok_or(OVERFLOW)? > MAX_OUTPUT_PIXELS
    {
        return Err(BAD_DIMENSIONS);
    }
    let row = width.checked_mul(channels).ok_or(OVERFLOW)?;
    let inflated = row.checked_add(1).and_then(|v| v.checked_mul(height)).ok_or(OVERFLOW)?;
    let output = ow.checked_mul(oh).and_then(|v| v.checked_mul(4)).ok_or(OVERFLOW)?;
    let scratch = row.checked_mul(2).ok_or(OVERFLOW)?;
    if inflated > MAX_INFLATED_BYTES { return Err(BAD_DIMENSIONS); }
    Ok((row, inflated, output, scratch))
}

/// Decode already-DEFLATE-inflated, non-interlaced PNG 8-bit RGB/RGBA scanlines
/// into a bounded RGBA8 surface using nearest-neighbor downsampling.
/// Scanlines include one PNG filter byte followed by exactly width*channels bytes.
///
/// Supported PNG color types: RGB8 (channels=3), RGBA8 (channels=4).
/// Unsupported: palette, grayscale, 16-bit, Adam7, color profiles, animation.
/// Another service must verify PNG chunks/CRC and perform bounded inflation.
///
/// Returns 0 for success, negative error codes on failure. No heap allocations.
/// The output is valid only on success. The caller supplies 2 scanlines of scratch.
/// All pointer/length regions must be valid, suitably aligned (u8), and disjoint.
///
/// # Safety
/// The caller must provide mapped, accessible, non-overlapping memory ranges of
/// the specified sizes, for the entire call. No pointer may alias another.
#[no_mangle]
pub unsafe extern "C" fn trinity_png_rows_to_rgba(
    inflated: *const u8,
    inflated_len: usize,
    width: u32,
    height: u32,
    channels: u32,
    rgba_out: *mut u8,
    rgba_out_len: usize,
    out_width: u32,
    out_height: u32,
    scratch: *mut u8,
    scratch_len: usize,
) -> i32 {
    if inflated.is_null() || rgba_out.is_null() || scratch.is_null() {
        return BAD_ARGUMENT;
    }

    let (width, height, channels, ow, oh) =
        (width as usize, height as usize, channels as usize,
         out_width as usize, out_height as usize);

    let (row_len, required_in, required_out, required_scratch) =
        match checked_sizes(width, height, channels, ow, oh) {
            Ok(v) => v,
            Err(e) => return e,
        };

    if inflated_len != required_in { return BAD_DATA_LENGTH; }
    if rgba_out_len < required_out || scratch_len < required_scratch {
        return BUFFER_TOO_SMALL;
    }

    // Check only the portions accessed, not unneeded capacity beyond them.
    let a = inflated as usize;
    let b = rgba_out as usize;
    let c = scratch as usize;
    let ab = ranges_overlap(a, required_in, b, required_out);
    let ac = ranges_overlap(a, required_in, c, required_scratch);
    let bc = ranges_overlap(b, required_out, c, required_scratch);
    if ab.is_err() || ac.is_err() || bc.is_err() { return OVERFLOW; }
    if ab.unwrap_or(true) || ac.unwrap_or(true) || bc.unwrap_or(true) {
        return ALIASING_REGIONS;
    }

    // SAFETY: The caller's pointer validity preconditions and disjoint-region
    // checks justify these bounded slices. No unbounded dynamic allocation.
    let input = unsafe { core::slice::from_raw_parts(inflated, required_in) };
    let out = unsafe { core::slice::from_raw_parts_mut(rgba_out, required_out) };
    let work = unsafe { core::slice::from_raw_parts_mut(scratch, required_scratch) };

    let (mut prev, mut current) = work.split_at_mut(row_len);
    prev.fill(0);
    let mut dst_y = 0_usize;

    for y in 0..height {
        let line = y * (row_len + 1);
        let filter = input[line];
        if filter > 4 { return BAD_FILTER; }
        let raw = &input[line + 1..line + 1 + row_len];
        for x in 0..row_len {
            let left = if x >= channels { current[x - channels] } else { 0 };
            let above = prev[x];
            let upper_left = if x >= channels { prev[x - channels] } else { 0 };
            let predictor = match filter {
                0 => 0,
                1 => left,
                2 => above,
                3 => ((left as u16 + above as u16) / 2) as u8,
                4 => paeth(left, above, upper_left),
                _ => return BAD_FILTER,
            };
            current[x] = raw[x].wrapping_add(predictor);
        }

        // Multiple output rows may sample the same input row when upscaling.
        while dst_y < oh && (dst_y * height / oh) == y {
            for dx in 0..ow {
                let sx = dx * width / ow;
                let si = sx * channels;
                let di = (dst_y * ow + dx) * 4;
                out[di] = current[si];
                out[di + 1] = current[si + 1];
                out[di + 2] = current[si + 2];
                out[di + 3] = if channels == 4 { current[si + 3] } else { 255 };
            }
            dst_y += 1;
        }
        mem::swap(&mut prev, &mut current);
    }

    0
}

#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo<'_>) -> ! {
    // Future Trinity process policy may use a supervisor trap instead.
    #[cfg(target_arch = "x86_64")]
    unsafe { core::arch::asm!("ud2", options(noreturn)); }
    #[cfg(not(target_arch = "x86_64"))]
    loop { core::hint::spin_loop(); }
}

#[cfg(test)]
extern crate std;

#[cfg(test)]
mod tests {
    use super::*;
    use std::vec;
    use std::vec::Vec;

    fn run(data: &[u8], width: u32, height: u32, channels: u32,
           ow: u32, oh: u32) -> (i32, Vec<u8>) {
        let mut out = vec![0; ow as usize * oh as usize * 4];
        let mut scratch = vec![0; width as usize * channels as usize * 2];
        let status = unsafe {
            trinity_png_rows_to_rgba(data.as_ptr(), data.len(), width, height, channels,
                out.as_mut_ptr(), out.len(), ow, oh,
                scratch.as_mut_ptr(), scratch.len())
        };
        (status, out)
    }

    #[test]
    fn rgba_single_pixel() {
        let (code, out) = run(&[0, 20, 30, 40, 50], 1, 1, 4, 1, 1);
        assert_eq!(code, 0);
        assert_eq!(out, [20,30,40,50]);
    }

    #[test]
    fn rgb_has_opaque_alpha() {
        let (code, out) = run(&[0, 1, 2, 3], 1, 1, 3, 1, 1);
        assert_eq!(code, 0);
        assert_eq!(out, [1,2,3,255]);
    }

    #[test]
    fn downsample_without_full_frame() {
        let source = [
            0, 10,11,12,13, 20,21,22,23,
            0, 30,31,32,33, 40,41,42,43,
        ];
        let (code, out) = run(&source, 2, 2, 4, 1, 1);
        assert_eq!(code, 0);
        assert_eq!(out, [10,11,12,13]);
    }

    #[test]
    fn png_sub_filter() {
        let (code, out) = run(&[1, 5,6,7, 5,6,7], 2, 1, 3, 2, 1);
        assert_eq!(code, 0);
        assert_eq!(out, [5,6,7,255,10,12,14,255]);
    }

    #[test]
    fn png_up_filter() {
        let (code, out) = run(&[0, 10,20,30, 2, 1,2,3], 1, 2, 3, 1, 2);
        assert_eq!(code, 0);
        assert_eq!(out, [10,20,30,255,11,22,33,255]);
    }

    #[test]
    fn png_average_filter() {
        let (code, out) = run(&[3, 8,10,12], 1, 1, 3, 1, 1);
        assert_eq!(code, 0);
        assert_eq!(out, [8,10,12,255]);
    }

    #[test]
    fn png_paeth_filter() {
        let (code, out) = run(&[0, 3,4,5, 4, 1,1,1], 1, 2, 3, 1, 2);
        assert_eq!(code, 0);
        assert_eq!(out, [3,4,5,255,4,5,6,255]);
    }

    #[test]
    fn bad_filter_rejected() {
        assert_eq!(run(&[5, 0,0,0,0], 1, 1, 4, 1, 1).0, BAD_FILTER);
    }

    #[test]
    fn bad_source_length_rejected() {
        assert_eq!(run(&[0,1], 1, 1, 4, 1, 1).0, BAD_DATA_LENGTH);
    }

    #[test]
    fn oversized_dimensions_rejected_before_touching_source() {
        assert_eq!(run(&[0], 8193, 1, 3, 1, 1).0, BAD_DIMENSIONS);
    }

    #[test]
    fn overlap_rejected() {
        let mut bytes = [0_u8; 32];
        let mut scratch = [0_u8; 8];
        let code = unsafe {
            trinity_png_rows_to_rgba(bytes.as_ptr(), 5, 1, 1, 4,
                bytes.as_mut_ptr(), 4, 1, 1, scratch.as_mut_ptr(), 8)
        };
        assert_eq!(code, ALIASING_REGIONS);
    }

    #[test]
    fn scratch_size_rejected() {
        let input = [0_u8; 5];
        let mut out = [0_u8; 4];
        let mut scratch = [0_u8; 7];
        let code = unsafe {
            trinity_png_rows_to_rgba(input.as_ptr(), 5, 1, 1, 4,
                out.as_mut_ptr(), 4, 1, 1, scratch.as_mut_ptr(), 7)
        };
        assert_eq!(code, BUFFER_TOO_SMALL);
    }
}
