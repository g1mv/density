use std::io::Result;
use std::io::Write;

use crate::algorithms::chameleon::{BYTE_SIZE_U128, BYTE_SIZE_U32, CHAMELEON_HASH_BITS, CHAMELEON_HASH_MULTIPLIER, Signature};

const FRAME_MAX_BYTE_SIZE: usize = 64 * BYTE_SIZE_U32;
const BIT_SIZE_U32: usize = 8 * BYTE_SIZE_U32;

pub struct ChameleonWriter<W: Write> {
    pub writer: W,
    pub dictionary: [u32; 1 << CHAMELEON_HASH_BITS],
    pub signature: Signature,
    pub frame_buffer: [u8; FRAME_MAX_BYTE_SIZE],
    pub frame_buffer_index: usize,
    pub input_buffer: [u8; BYTE_SIZE_U128],
    pub input_buffer_index: usize,
    pub is_started: bool,
    pub is_flushed: bool,
}

impl<W: Write> ChameleonWriter<W> {
    pub fn new(writer: W) -> Self {
        ChameleonWriter {
            writer,
            dictionary: [0; 1 << CHAMELEON_HASH_BITS],
            signature: Signature { value: 0, shift: 0 },
            frame_buffer: [0; FRAME_MAX_BYTE_SIZE],
            frame_buffer_index: 0,
            input_buffer: [0; BYTE_SIZE_U128],
            input_buffer_index: 0,
            is_started: false,
            is_flushed: false,
        }
    }

    #[inline(always)]
    pub fn shift_signature(&mut self) -> bool {
        if self.signature.shift == 0x3f {
            self.signature.shift = 0;
            true
        } else {
            self.signature.shift += 1;
            false
        }
    }

    #[inline(always)]
    pub fn push_signature_bit(&mut self, bit: u64) -> bool {
        self.signature.value |= bit << self.signature.shift;
        self.shift_signature()
    }

    #[inline(always)]
    pub fn push_bytes_to_frame_buffer(&mut self, bytes: &[u8]) {
        self.frame_buffer[self.frame_buffer_index..self.frame_buffer_index + bytes.len()].copy_from_slice(&bytes);
        self.frame_buffer_index += bytes.len();
    }

    #[inline(always)]
    pub fn reset_frame_buffer(&mut self) {
        self.frame_buffer_index = 0;
    }

    #[inline(always)]
    pub fn write_frame_buffer(&mut self) {
        self.writer.write_all(&self.signature.value.to_le_bytes()).expect("ouch");
        self.writer.write_all(&self.frame_buffer[0..self.frame_buffer_index]).expect("ouch");
    }

    #[inline(always)]
    pub fn kernel(&mut self, value_u32: u32) {
        let hash_u16 = ((value_u32 * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
        let dictionary_value = &mut self.dictionary[hash_u16 as usize];
        if &value_u32 == dictionary_value {
            self.push_bytes_to_frame_buffer(&hash_u16.to_le_bytes());
            if self.push_signature_bit(1) {
                self.write_frame_buffer();
                self.reset_frame_buffer();
            }
        } else {
            *dictionary_value = value_u32;
            self.push_bytes_to_frame_buffer(&value_u32.to_le_bytes());
            if self.push_signature_bit(0) {
                self.write_frame_buffer();
                self.reset_frame_buffer();
            }
        }
    }

    #[inline(always)]
    pub fn process_chunk(&mut self, value_u128: u128) {
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
}

impl<W: Write> Drop for ChameleonWriter<W> {
    fn drop(&mut self) {
        // Ignore errors
        let _ = self.flush();
    }
}

impl<W: Write> Write for ChameleonWriter<W> {
    fn write(&mut self, input: &[u8]) -> Result<usize> {
        if input.len() > 0 {
            self.is_started = true;
            let mut input_index = 0;

            // Add to input buffer if not empty
            if self.input_buffer_index > 0 {
                let fill_bytes = BYTE_SIZE_U128 - self.input_buffer_index;
                if input.len() < fill_bytes {
                    self.input_buffer[self.input_buffer_index..self.input_buffer_index + input.len()].copy_from_slice(&input);
                    return Ok(input.len());
                } else {
                    self.input_buffer[self.input_buffer_index..BYTE_SIZE_U128].copy_from_slice(&input[0..fill_bytes]);
                    self.process_chunk(u128::from_ne_bytes(self.input_buffer.try_into().unwrap()));

                    self.input_buffer_index = 0;
                    input_index = fill_bytes;
                }
            }

            // Process chunks
            while input_index + BYTE_SIZE_U128 <= input.len() {
                self.process_chunk(u128::from_ne_bytes(input[input_index..input_index + BYTE_SIZE_U128].try_into().unwrap()));

                input_index += BYTE_SIZE_U128;
            }

            // Store remaining bytes in input buffer
            self.input_buffer_index = input.len() - input_index;
            self.input_buffer[0..self.input_buffer_index].copy_from_slice(&input[input_index..input.len()]);
        }

        Ok(input.len())
    }

    fn flush(&mut self) -> Result<()> {
        if self.is_started {
            if !self.is_flushed {
                if self.signature.shift > 0 {
                    self.write_frame_buffer();
                }
                if self.input_buffer_index > 0 {
                    self.writer.write_all(&self.input_buffer[0..self.input_buffer_index]).expect("ouch");
                    self.input_buffer_index = 0;
                }
                self.is_flushed = true;
            }
        }
        Ok(())
    }
}