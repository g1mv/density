use std::mem::size_of;
use std::ptr::write_unaligned;
use std::slice::from_raw_parts;

use crate::codec::Codec;
use crate::error::Error;

pub(crate) const CHAMELEON_HASH_BITS: usize = 16;
pub(crate) const CHAMELEON_HASH_MULTIPLIER: u32 = 0x9D6EF916;
pub(crate) const BYTE_SIZE_U16: usize = size_of::<u16>();
pub(crate) const BYTE_SIZE_U32: usize = size_of::<u32>();
pub(crate) const BYTE_SIZE_U64: usize = size_of::<u64>();
pub(crate) const BYTE_SIZE_U128: usize = size_of::<u128>();

pub struct Signature {
    pub(crate) value: u64,
    pub(crate) shift: u8,
}

pub struct Chameleon {
    pub dictionary: [u32; 1 << CHAMELEON_HASH_BITS],
    pub signature: Signature,
    pub index: u64,
}

impl Chameleon {
    pub fn new() -> Self {
        Chameleon {
            dictionary: [0; 1 << CHAMELEON_HASH_BITS],
            signature: Signature { value: 0, shift: 0 },
            index: 0,
        }
    }

    #[inline(always)]
    pub fn shift_sgn(&mut self) -> bool {
        if self.signature.shift == 0x3f {
            self.signature.shift = 0;
            true
        } else {
            self.signature.shift += 1;
            false
        }
    }

    #[inline(always)]
    pub fn push_sgn_bit(&mut self, bit: u64) -> bool {
        self.signature.value |= bit << self.signature.shift;
        self.shift_sgn()
    }

    #[inline(always)]
    pub fn encode_quad(&mut self, value_u32: u32, output: &mut [u8], out_index: &mut usize, sgn_index: &mut usize) {
        let hash_u16 = ((value_u32 * CHAMELEON_HASH_MULTIPLIER) >> (32 - CHAMELEON_HASH_BITS)) as u16;
        let dictionary_value = &mut self.dictionary[hash_u16 as usize];
        if &value_u32 == dictionary_value {
            if self.push_sgn_bit(1) {
                output[*sgn_index..*sgn_index + BYTE_SIZE_U64].copy_from_slice(&self.signature.value.to_le_bytes());
                *sgn_index = *out_index;
                *out_index += BYTE_SIZE_U64;
            }
            output[*out_index..*out_index + BYTE_SIZE_U16].copy_from_slice(&hash_u16.to_le_bytes());
            *out_index += BYTE_SIZE_U16;
        } else {
            *dictionary_value = value_u32;
            if self.push_sgn_bit(0) {
                output[*sgn_index..*sgn_index + BYTE_SIZE_U64].copy_from_slice(&self.signature.value.to_le_bytes());
                *sgn_index = *out_index;
                *out_index += BYTE_SIZE_U64;
            }
            output[*out_index..*out_index + BYTE_SIZE_U32].copy_from_slice(&value_u32.to_le_bytes());
            *out_index += BYTE_SIZE_U32;
        }
    }

    #[inline(always)]
    pub unsafe fn unsafe_encode_quad(&mut self, value_u32: u32, out_ptr: &mut *mut u8, sgn_ptr: &mut *mut u64) {
        let hash_u16 = ((value_u32 * CHAMELEON_HASH_MULTIPLIER) >> (32 - CHAMELEON_HASH_BITS)) as u16;
        let dictionary_value = &mut self.dictionary[hash_u16 as usize];
        if &value_u32 == dictionary_value {
            if self.push_sgn_bit(1) {
                write_unaligned(*sgn_ptr, self.signature.value);
                *sgn_ptr = *out_ptr as *mut u64;
                *out_ptr = out_ptr.add(BYTE_SIZE_U64);
            }
            let out_ptr_u16 = *out_ptr as *mut u16;
            write_unaligned(out_ptr_u16, hash_u16);
            *out_ptr = out_ptr.add(BYTE_SIZE_U16);
        } else {
            *dictionary_value = value_u32;
            if self.push_sgn_bit(0) {
                write_unaligned(*sgn_ptr, self.signature.value);
                *sgn_ptr = *out_ptr as *mut u64;
                *out_ptr = out_ptr.add(BYTE_SIZE_U64);
            }
            let out_ptr_u32 = *out_ptr as *mut u32;
            write_unaligned(out_ptr_u32, value_u32);
            *out_ptr = out_ptr.add(BYTE_SIZE_U32);
        }
    }

    pub fn encode_safe(&mut self, input: &[u8], output: &mut [u8]) -> Result<usize, Error> {
        let start_index = 0;
        let mut sgn_index = 0;
        let mut out_index = BYTE_SIZE_U64;
        for bytes in input.chunks(BYTE_SIZE_U128) {
            match <&[u8] as TryInto<[u8; BYTE_SIZE_U128]>>::try_into(bytes) {
                Ok(array) => {
                    let value_u128 = u128::from_ne_bytes(array);

                    #[cfg(target_endian = "little")]
                    {
                        self.encode_quad((value_u128 >> 96) as u32, output, &mut out_index, &mut sgn_index);
                        self.encode_quad(((value_u128 >> 64) & 0xffffffff) as u32, output, &mut out_index, &mut sgn_index);
                        self.encode_quad(((value_u128 >> 32) & 0xffffffff) as u32, output, &mut out_index, &mut sgn_index);
                        self.encode_quad((value_u128 & 0xffffffff) as u32, output, &mut out_index, &mut sgn_index);
                    }

                    #[cfg(target_endian = "big")]
                    {
                        self.encode_quad((value_u128 & 0xffffffff) as u32, output, &mut out_index, &mut sgn_index);
                        self.encode_quad(((value_u128 >> 32) & 0xffffffff) as u32, output, &mut out_index, &mut sgn_index);
                        self.encode_quad(((value_u128 >> 64) & 0xffffffff) as u32, output, &mut out_index, &mut sgn_index);
                        self.encode_quad((value_u128 >> 96) as u32, output, &mut out_index, &mut sgn_index);
                    }
                }
                Err(_error) => {
                    output[out_index..out_index + bytes.len()].copy_from_slice(bytes);
                    out_index += bytes.len();
                    println!("Ending");
                }
            }
        }
        if self.signature.shift > 0 {
            output[sgn_index..sgn_index + BYTE_SIZE_U64].copy_from_slice(&self.signature.value.to_le_bytes());
        } else {
            // Signature reserved space has not been used
            out_index -= BYTE_SIZE_U64;
        }
        // println!("{}", out_index - start_index);
        Ok(out_index - start_index)
    }
}

impl Codec for Chameleon {
    unsafe fn encode_unsafe(&mut self, input: &[u8], output: &mut [u8]) -> Result<usize, Error> {
        let in_ptr = input.as_ptr();
        let in_len = input.len();
        let origin_ptr = output.as_ptr();
        let mut out_ptr = output.as_mut_ptr();
        let mut sgn_ptr = out_ptr as *mut u64;
        out_ptr = out_ptr.add(size_of::<u64>() / size_of::<u8>());

        let in_ptr_u32 = in_ptr as *const u32;
        let in_len_u32 = in_len >> 2;
        let in_array_u32 = from_raw_parts(in_ptr_u32, in_len_u32);
        for value_u32 in in_array_u32 {
            let hash_u16 = ((value_u32 * CHAMELEON_HASH_MULTIPLIER) >> (32 - CHAMELEON_HASH_BITS)) as u16;
            let dictionary_value = &mut self.dictionary[hash_u16 as usize];
            if value_u32 == dictionary_value {
                if self.push_sgn_bit(1) {
                    write_unaligned(sgn_ptr, self.signature.value);
                    sgn_ptr = out_ptr as *mut u64;
                    out_ptr = out_ptr.add(size_of::<u64>() / size_of::<u8>());
                }
                let out_ptr_u16 = out_ptr as *mut u16;
                write_unaligned(out_ptr_u16, hash_u16);
                out_ptr = out_ptr.add(size_of::<u16>() / size_of::<u8>());
            } else {
                *dictionary_value = *value_u32;
                if self.push_sgn_bit(0) {
                    write_unaligned(sgn_ptr, self.signature.value);
                    sgn_ptr = out_ptr as *mut u64;
                    out_ptr = out_ptr.add(size_of::<u64>() / size_of::<u8>());
                }
                let out_ptr_u32 = out_ptr as *mut u32;
                write_unaligned(out_ptr_u32, *value_u32);
                out_ptr = out_ptr.add(size_of::<u32>() / size_of::<u8>());
            }
        }

        // let in_ptr_u128 = in_ptr as *const u128;
        // let in_len_u128 = in_len >> 4;
        // let in_array_u128 = from_raw_parts(in_ptr_u128, in_len_u128);
        // for value_u128 in in_array_u128 {
        //     #[cfg(target_endian = "little")]
        //     {
        //         self.unsafe_encode_quad((value_u128 >> 96) as u32, &mut out_ptr, &mut sgn_ptr);
        //         self.unsafe_encode_quad(((value_u128 >> 64) & 0xffffffff) as u32, &mut out_ptr, &mut sgn_ptr);
        //         self.unsafe_encode_quad(((value_u128 >> 32) & 0xffffffff) as u32, &mut out_ptr, &mut sgn_ptr);
        //         self.unsafe_encode_quad((value_u128 & 0xffffffff) as u32, &mut out_ptr, &mut sgn_ptr);
        //     }
        //
        //     #[cfg(target_endian = "big")]
        //     {
        //         self.unsafe_encode_quad((value_u128 & 0xffffffff) as u32, &mut out_ptr, &mut sgn_ptr);
        //         self.unsafe_encode_quad(((value_u128 >> 32) & 0xffffffff) as u32, &mut out_ptr, &mut sgn_ptr);
        //         self.unsafe_encode_quad(((value_u128 >> 64) & 0xffffffff) as u32, &mut out_ptr, &mut sgn_ptr);
        //         self.unsafe_encode_quad((value_u128 >> 96) as u32, &mut out_ptr, &mut sgn_ptr);
        //     }
        // }

        write_unaligned(sgn_ptr, self.signature.value);
        // println!("{} bytes", out_ptr.offset_from(origin_ptr));
        Ok(out_ptr.offset_from(origin_ptr) as usize)
    }

    unsafe fn decode(&mut self, _input: &[u8], _output: &mut [u8]) -> Result<(), Error> {
        todo!()
    }
}