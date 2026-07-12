//! Stable C ABI between the 7-Zip C++ shim and the Rust Density crate.
//!
//! Panics are caught here so Rust unwinding never crosses into C++/7-Zip.

use density::algorithms::chameleon::chameleon::Chameleon;
use density::algorithms::cheetah::cheetah::Cheetah;
use density::algorithms::lion::lion::Lion;
use density::codec::codec::Codec;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr::NonNull;
use std::slice;

pub const ALGORITHM_CHAMELEON: u32 = 1;
pub const ALGORITHM_CHEETAH: u32 = 2;
pub const ALGORITHM_LION: u32 = 3;

pub const STATUS_OK: i32 = 0;
pub const STATUS_INVALID_ARGUMENT: i32 = 1;
pub const STATUS_UNSUPPORTED_ALGORITHM: i32 = 2;
pub const STATUS_CODEC_ERROR: i32 = 3;
pub const STATUS_PANIC: i32 = 4;

fn encode_bound(algorithm: u32, input_size: usize) -> Option<usize> {
    match algorithm {
        ALGORITHM_CHAMELEON => Some(Chameleon::safe_encode_buffer_size(input_size)),
        ALGORITHM_CHEETAH => Some(Cheetah::safe_encode_buffer_size(input_size)),
        ALGORITHM_LION => Some(Lion::safe_encode_buffer_size(input_size)),
        _ => None,
    }
}

fn encode(algorithm: u32, input: &[u8], output: &mut [u8]) -> Result<usize, i32> {
    let result = match algorithm {
        ALGORITHM_CHAMELEON => Chameleon::encode(input, output),
        ALGORITHM_CHEETAH => Cheetah::encode(input, output),
        ALGORITHM_LION => Lion::encode(input, output),
        _ => return Err(STATUS_UNSUPPORTED_ALGORITHM),
    };
    result.map_err(|_| STATUS_CODEC_ERROR)
}

fn decode(algorithm: u32, input: &[u8], output: &mut [u8]) -> Result<usize, i32> {
    let result = match algorithm {
        ALGORITHM_CHAMELEON => Chameleon::decode(input, output),
        ALGORITHM_CHEETAH => Cheetah::decode(input, output),
        ALGORITHM_LION => Lion::decode(input, output),
        _ => return Err(STATUS_UNSUPPORTED_ALGORITHM),
    };
    result.map_err(|_| STATUS_CODEC_ERROR)
}

unsafe fn input_slice<'a>(ptr: *const u8, len: usize) -> &'a [u8] {
    let ptr = if len == 0 {
        NonNull::<u8>::dangling().as_ptr() as *const u8
    } else {
        ptr
    };
    // SAFETY: exported functions validate non-null pointers for non-empty
    // slices, and the C++ caller keeps the allocation alive for the call.
    unsafe { slice::from_raw_parts(ptr, len) }
}

unsafe fn output_slice<'a>(ptr: *mut u8, len: usize) -> &'a mut [u8] {
    let ptr = if len == 0 {
        NonNull::<u8>::dangling().as_ptr()
    } else {
        ptr
    };
    // SAFETY: exported functions validate non-null pointers for non-empty
    // slices, and the C++ caller provides unique writable storage.
    unsafe { slice::from_raw_parts_mut(ptr, len) }
}

#[unsafe(no_mangle)]
pub extern "C" fn density_plugin_bound(algorithm: u32, input_size: usize) -> usize {
    catch_unwind(AssertUnwindSafe(|| encode_bound(algorithm, input_size).unwrap_or(0)))
        .unwrap_or(0)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn density_plugin_encode(
    algorithm: u32,
    input: *const u8,
    input_size: usize,
    output: *mut u8,
    output_size: usize,
    written: *mut usize,
) -> i32 {
    if written.is_null()
        || (input_size != 0 && input.is_null())
        || (output_size != 0 && output.is_null())
    {
        return STATUS_INVALID_ARGUMENT;
    }

    // SAFETY: `written` was checked above and belongs to the caller.
    unsafe { *written = 0 };

    match catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: pointer/length pairs were checked above.
        let input = unsafe { input_slice(input, input_size) };
        let output = unsafe { output_slice(output, output_size) };
        encode(algorithm, input, output)
    })) {
        Ok(Ok(size)) => {
            // SAFETY: `written` remains valid for this call.
            unsafe { *written = size };
            STATUS_OK
        }
        Ok(Err(status)) => status,
        Err(_) => STATUS_PANIC,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn density_plugin_decode(
    algorithm: u32,
    input: *const u8,
    input_size: usize,
    output: *mut u8,
    output_size: usize,
    written: *mut usize,
) -> i32 {
    if written.is_null()
        || (input_size != 0 && input.is_null())
        || (output_size != 0 && output.is_null())
    {
        return STATUS_INVALID_ARGUMENT;
    }

    // SAFETY: `written` was checked above and belongs to the caller.
    unsafe { *written = 0 };

    match catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: pointer/length pairs were checked above.
        let input = unsafe { input_slice(input, input_size) };
        let output = unsafe { output_slice(output, output_size) };
        decode(algorithm, input, output)
    })) {
        Ok(Ok(size)) => {
            // SAFETY: `written` remains valid for this call.
            unsafe { *written = size };
            STATUS_OK
        }
        Ok(Err(status)) => status,
        Err(_) => STATUS_PANIC,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn round_trip(algorithm: u32) {
        let input = b"Density 7-Zip bridge test data. Density 7-Zip bridge test data."
            .repeat(4096);
        let mut packed = vec![0_u8; encode_bound(algorithm, input.len()).unwrap()];
        let packed_size = encode(algorithm, &input, &mut packed).unwrap();
        let mut decoded = vec![0_u8; input.len()];
        let decoded_size = decode(algorithm, &packed[..packed_size], &mut decoded).unwrap();
        assert_eq!(decoded_size, input.len());
        assert_eq!(decoded, input);
    }

    #[test]
    fn chameleon_round_trip() {
        round_trip(ALGORITHM_CHAMELEON);
    }

    #[test]
    fn cheetah_round_trip() {
        round_trip(ALGORITHM_CHEETAH);
    }

    #[test]
    fn lion_round_trip() {
        round_trip(ALGORITHM_LION);
    }

    #[test]
    fn ffi_round_trip() {
        let input = b"ffi bridge ffi bridge ffi bridge".repeat(1024);
        let mut packed = vec![0_u8; density_plugin_bound(ALGORITHM_CHEETAH, input.len())];
        let mut packed_size = 0_usize;
        let encode_status = unsafe {
            density_plugin_encode(
                ALGORITHM_CHEETAH,
                input.as_ptr(),
                input.len(),
                packed.as_mut_ptr(),
                packed.len(),
                &mut packed_size,
            )
        };
        assert_eq!(encode_status, STATUS_OK);

        let mut decoded = vec![0_u8; input.len()];
        let mut decoded_size = 0_usize;
        let decode_status = unsafe {
            density_plugin_decode(
                ALGORITHM_CHEETAH,
                packed.as_ptr(),
                packed_size,
                decoded.as_mut_ptr(),
                decoded.len(),
                &mut decoded_size,
            )
        };
        assert_eq!(decode_status, STATUS_OK);
        assert_eq!(decoded_size, input.len());
        assert_eq!(decoded, input);
    }

    #[test]
    fn ffi_rejects_invalid_arguments() {
        let status = unsafe {
            density_plugin_encode(
                ALGORITHM_CHAMELEON,
                std::ptr::null(),
                1,
                std::ptr::null_mut(),
                0,
                std::ptr::null_mut(),
            )
        };
        assert_eq!(status, STATUS_INVALID_ARGUMENT);
        assert_eq!(density_plugin_bound(99, 1024), 0);
    }
}
