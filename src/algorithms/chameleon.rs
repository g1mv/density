use std::mem::size_of;
use std::ops::{Shl, Shr};
use std::ptr::write_unaligned;
use std::slice::from_raw_parts;
use crate::codec::Codec;
use crate::error::Error;

const CHAMELEON_HASH_BITS: u64 = 16;
const CHAMELEON_HASH_MULTIPLIER: u32 = 0x9D6EF916;

pub struct Signature {
    value: u64,
    shift: u8,
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
    pub fn push_sgn_bit(&mut self, bit: u64) -> bool {
        self.signature.value |= bit << self.signature.shift;
        if self.signature.shift == 0x3f {
            self.signature.shift = 0;
            true
        } else {
            false
        }
    }

    fn read_le_u32(input: &mut &[u8]) -> u32 {
        let (int_bytes, rest) = input.split_at(size_of::<u32>());
        *input = rest;
        u32::from_le_bytes(int_bytes.try_into().unwrap())
    }

    pub fn encode_safe(&mut self, input: &[u8], output: &mut [u8]) -> Result<(), Error> {
        let mut sgn_index = 0;
        let mut out_index = size_of::<u64>() / size_of::<u8>();
        for quad_bytes in input.chunks(4) {
            let value_u32 = u32::from_le_bytes(quad_bytes.try_into().unwrap()); // (quad_bytes[0] as u32).shl(24) + (quad_bytes[1] as u32).shl(16) + (quad_bytes[2] as u32).shl(8) + quad_bytes[3] as u32;
            let hash_u16 = (&value_u32 * CHAMELEON_HASH_MULTIPLIER).shr(32 - CHAMELEON_HASH_BITS) as u16;
            let mut dictionary_value = &mut self.dictionary[hash_u16 as usize];
            if &value_u32 == dictionary_value {
                if self.push_sgn_bit(1) {
                    output[sgn_index..sgn_index + 8].copy_from_slice(&self.signature.value.to_le_bytes());
                    sgn_index = out_index;
                    out_index += size_of::<u64>() / size_of::<u8>();
                }
                output[out_index..out_index + 2].copy_from_slice(&hash_u16.to_le_bytes());
                out_index += size_of::<u16>() / size_of::<u8>();
            } else {
                *dictionary_value = value_u32;
                if self.push_sgn_bit(0) {
                    output[sgn_index..sgn_index + 8].copy_from_slice(&self.signature.value.to_le_bytes());
                    sgn_index = out_index;
                    out_index += size_of::<u64>() / size_of::<u8>();
                }
                output[out_index..out_index + 4].copy_from_slice(&value_u32.to_le_bytes());
                out_index += size_of::<u32>() / size_of::<u8>();
            }
        }
        output[sgn_index..sgn_index + 8].copy_from_slice(&self.signature.value.to_le_bytes());
        Ok(())
    }
}

impl Codec for Chameleon {
    unsafe fn encode(&mut self, input: &[u8], output: &mut [u8]) -> Result<(), Error> {
        return self.encode_safe(input, output);

        // let in_ptr = input.as_ptr();
        // let in_len = input.len();
        // // let origin_ptr = output.as_ptr();
        // let mut out_ptr = output.as_mut_ptr();
        // let mut sgn_ptr = out_ptr as *mut u64;
        // out_ptr = out_ptr.add(size_of::<u64>() / size_of::<u8>());
        // let in_ptr_u32 = in_ptr as *const u32;
        // let in_len_u32 = in_len >> 2;
        // let in_array_u32 = from_raw_parts(in_ptr_u32, in_len_u32);
        // for value_u32 in in_array_u32 {
        //     let hash_u16 = ((value_u32 * CHAMELEON_HASH_MULTIPLIER) >> (32 - CHAMELEON_HASH_BITS)) as u16;
        //     let mut dictionary_value = &mut self.dictionary[hash_u16 as usize];
        //     if value_u32 == dictionary_value {
        //         if self.push_sgn_bit(1) {
        //             write_unaligned(sgn_ptr, self.signature.value);
        //             sgn_ptr = out_ptr as *mut u64;
        //             out_ptr = out_ptr.add(size_of::<u64>() / size_of::<u8>());
        //         }
        //         let out_ptr_u16 = out_ptr as *mut u16;
        //         write_unaligned(out_ptr_u16, hash_u16);
        //         out_ptr = out_ptr.add(size_of::<u16>() / size_of::<u8>());
        //     } else {
        //         *dictionary_value = *value_u32;
        //         if self.push_sgn_bit(0) {
        //             write_unaligned(sgn_ptr, self.signature.value);
        //             sgn_ptr = out_ptr as *mut u64;
        //             out_ptr = out_ptr.add(size_of::<u64>() / size_of::<u8>());
        //         }
        //         let out_ptr_u32 = out_ptr as *mut u32;
        //         write_unaligned(out_ptr_u32, *value_u32);
        //         out_ptr = out_ptr.add(size_of::<u32>() / size_of::<u8>());
        //     }
        // }
        // write_unaligned(sgn_ptr, self.signature.value);
        // // println!("{} bytes", out_ptr.offset_from(origin_ptr));
        // Ok(())
    }

    unsafe fn decode(&mut self, input: &[u8], output: &mut [u8]) -> Result<(), Error> {
        todo!()
    }
}