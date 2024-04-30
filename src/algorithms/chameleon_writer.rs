use std::io::Result;
use std::io::Write;
use std::ops::Index;

use crate::algorithms::chameleon::{BYTE_SIZE_U128, BYTE_SIZE_U32, BYTE_SIZE_U64, CHAMELEON_HASH_BITS, CHAMELEON_HASH_MULTIPLIER, Signature};

const BYTE_SIZE_FRAME: usize = BYTE_SIZE_U64 + 64 * BYTE_SIZE_U32;
const NUM_SUBFRAMES: usize = 1;

struct Slice<'a, T: 'a>(&'a [T], &'a [T]);

impl<'a, T: 'a> Index<usize> for Slice<'a, T> {
    type Output = T;
    fn index(&self, index: usize) -> &T {
        if index < self.0.len() {
            &self.0[index]
        } else {
            &self.1[index - self.0.len()]
        }
    }
}

pub struct Sig128 {
    pub(crate) value: u128,
    pub(crate) shift: u8,
}

pub struct ChameleonWriter<W: Write> {
    pub writer: W,
    pub dictionary: [u32; 1 << CHAMELEON_HASH_BITS],
    pub signature: Signature,
    // pub sgn_index: usize,
    // pub out_index: usize,
    // pub temp_buffer: [u8; BYTE_SIZE_U128],
    // pub temp_size: usize,
    pub frame: [u8; NUM_SUBFRAMES * BYTE_SIZE_FRAME],
    pub sgn_index: usize,
    pub frm_index: usize,
    pub frm_num: usize,
}

impl<W: Write> ChameleonWriter<W> {
    pub fn new(writer: W) -> Self {
        ChameleonWriter {
            writer,
            dictionary: [0; 1 << CHAMELEON_HASH_BITS],
            signature: Signature { value: 0, shift: 0 },
            // sgn_index: 0,
            // out_index: BYTE_SIZE_U64,
            // temp_buffer: [0; BYTE_SIZE_U128],
            // temp_size: 0,
            frame: [0; NUM_SUBFRAMES * BYTE_SIZE_FRAME],
            frm_index: BYTE_SIZE_U64,
            sgn_index: 0,
            frm_num: 0,
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
    pub fn push_to_frame(&mut self, bytes: &[u8]) {
        self.frame[self.frm_index..self.frm_index + bytes.len()].copy_from_slice(&bytes);
        self.frm_index += bytes.len();
    }

    #[inline(always)]
    pub fn complete_frame(&mut self) -> bool {
        self.frame[self.sgn_index..self.sgn_index + BYTE_SIZE_U64].copy_from_slice(&self.signature.value.to_le_bytes());
        if self.frm_num >= NUM_SUBFRAMES - 1 {
            self.frm_num = 0;
            self.sgn_index = 0;
            self.frm_index = BYTE_SIZE_U64;
            true
        } else {
            self.sgn_index = self.frm_index;
            self.frm_num += 1;
            false
        }
    }

    #[inline(always)]
    pub fn kernel(&mut self, value_u32: u32) {
        let hash_u16 = ((value_u32 * CHAMELEON_HASH_MULTIPLIER) >> (32 - CHAMELEON_HASH_BITS)) as u16;
        let dictionary_value = &mut self.dictionary[hash_u16 as usize];
        if &value_u32 == dictionary_value {
            if self.push_sgn_bit(1) {
                if self.complete_frame() {
                    self.writer.write(&self.frame[0..self.frm_index]).expect("ouch");
                }
                // self.buffer[self.sgn_index..self.sgn_index + BYTE_SIZE_U64].copy_from_slice(&self.signature.value.to_le_bytes());
                // self.sgn_index = self.out_index;
                // self.out_index += BYTE_SIZE_U64;
            }
            self.push_to_frame(&hash_u16.to_le_bytes());
            // self.buffer[self.out_index..self.out_index + BYTE_SIZE_U16].copy_from_slice(&hash_u16.to_le_bytes());
            // self.out_index += BYTE_SIZE_U16;
        } else {
            *dictionary_value = value_u32;
            if self.push_sgn_bit(0) {
                if self.complete_frame() {
                    self.writer.write(&self.frame[0..self.frm_index]).expect("ouch");
                }
                // self.buffer[self.sgn_index..self.sgn_index + BYTE_SIZE_U64].copy_from_slice(&self.signature.value.to_le_bytes());
                // self.sgn_index = self.out_index;
                // self.out_index += BYTE_SIZE_U64;
            }
            self.push_to_frame(&value_u32.to_le_bytes());
            // self.buffer[self.out_index..self.out_index + BYTE_SIZE_U32].copy_from_slice(&value_u32.to_le_bytes());
            // self.out_index += BYTE_SIZE_U32;
        }
    }
}

impl<W: Write> Write for ChameleonWriter<W> {
    fn write(&mut self, input: &[u8]) -> Result<usize> {
        // println!("{}", input.len());

        // let start_index = 0;
        // let mut shift_index = 0;    // BYTE_SIZE_U128 - self.temp_size;

        // if self.temp_size != 0 {
        //     self.temp_buffer[self.temp_size..BYTE_SIZE_U128].copy_from_slice(&input[0..shift_index]);
        //     self.temp_size = 0;
        //
        //     let value_u128 = u128::from_ne_bytes(self.temp_buffer.try_into().unwrap());
        //     #[cfg(target_endian = "little")]
        //     {
        //         self.kernel((value_u128 >> 96) as u32);
        //         self.kernel(((value_u128 >> 64) & 0xffffffff) as u32);
        //         self.kernel(((value_u128 >> 32) & 0xffffffff) as u32);
        //         self.kernel((value_u128 & 0xffffffff) as u32);
        //     }
        //
        //     #[cfg(target_endian = "big")]
        //     {
        //         self.kernel((value_u128 & 0xffffffff) as u32);
        //         self.kernel(((value_u128 >> 32) & 0xffffffff) as u32);
        //         self.kernel(((value_u128 >> 64) & 0xffffffff) as u32);
        //         self.kernel((value_u128 >> 96) as u32);
        //     }
        // }

        // while shift_index + BYTE_SIZE_U128 <= input.len() {
        //     let value_u128 = u128::from_ne_bytes(input[shift_index..shift_index + BYTE_SIZE_U128].try_into().unwrap());
        //
        //     #[cfg(target_endian = "little")]
        //     {
        //         self.kernel((value_u128 >> 96) as u32);
        //         self.kernel(((value_u128 >> 64) & 0xffffffff) as u32);
        //         self.kernel(((value_u128 >> 32) & 0xffffffff) as u32);
        //         self.kernel((value_u128 & 0xffffffff) as u32);
        //     }
        //
        //     #[cfg(target_endian = "big")]
        //     {
        //         self.kernel((value_u128 & 0xffffffff) as u32);
        //         self.kernel(((value_u128 >> 32) & 0xffffffff) as u32);
        //         self.kernel(((value_u128 >> 64) & 0xffffffff) as u32);
        //         self.kernel((value_u128 >> 96) as u32);
        //     }
        //
        //     shift_index += BYTE_SIZE_U128;
        // }

        // println!("done");

        for bytes in input.chunks(BYTE_SIZE_U128) {
            match <&[u8] as TryInto<[u8; BYTE_SIZE_U128]>>::try_into(bytes) {
                Ok(array) => {
                    let value_u128 = u128::from_ne_bytes(array);

                    #[cfg(target_endian = "little")]
                    {
                        self.kernel((value_u128 >> 96) as u32);
                        self.kernel(((value_u128 >> 64) & 0xffffffff) as u32);
                        self.kernel(((value_u128 >> 32) & 0xffffffff) as u32);
                        self.kernel((value_u128 & 0xffffffff) as u32);
                    }

                    #[cfg(target_endian = "big")]
                    {
                        self.kernel((value_u128 & 0xffffffff) as u32);
                        self.kernel(((value_u128 >> 32) & 0xffffffff) as u32);
                        self.kernel(((value_u128 >> 64) & 0xffffffff) as u32);
                        self.kernel((value_u128 >> 96) as u32);
                    }
                }
                Err(_error) => {
                    println!("Ending");
                    return Ok(input.len() - input.len() % BYTE_SIZE_U128);
                }
            }
        }

        // self.buffer[self.sgn_index..self.sgn_index + BYTE_SIZE_U64].copy_from_slice(&self.signature.value.to_le_bytes());

        // println!("{} bytes", self.out_index - start_index);
        Ok(input.len())
    }

    fn flush(&mut self) -> Result<()> {
        // self.buffer[self.sgn_index..self.sgn_index + BYTE_SIZE_U64].copy_from_slice(&self.signature.value.to_le_bytes());
        // self.final_index = self.out_index;
        Ok(())
    }
}