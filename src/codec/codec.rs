use crate::codec::quad_decoder::QuadDecoder;
use crate::codec::quad_encoder::QuadEncoder;
use crate::errors::decode_error::DecodeError;
use crate::errors::encode_error::EncodeError;
use crate::io::read_buffer::ReadBuffer;
use crate::io::read_signature::ReadSignature;
use crate::io::write_buffer::WriteBuffer;
use crate::io::write_signature::WriteSignature;
use crate::{BYTE_SIZE_U128, BYTE_SIZE_U32, BYTE_SIZE_U64};

pub struct ProtectionState {
    pub(crate) copy_penalty: u8,
    pub(crate) copy_penalty_start: u8,
    pub(crate) previous_incompressible: bool,
    pub(crate) counter: u64,
}

impl ProtectionState {
    pub fn new() -> Self {
        ProtectionState {
            copy_penalty: 0,
            copy_penalty_start: 1,
            previous_incompressible: false,
            counter: 0,
        }
    }

    #[inline(always)]
    pub fn revert_to_copy(&mut self) -> bool {
        if self.counter & 0xf == 0 {
            if self.copy_penalty_start > 1 {
                self.copy_penalty_start >>= 1;
            }
        }
        self.counter += 1;
        self.copy_penalty > 0
    }

    #[inline(always)]
    pub fn decay(&mut self) {
        self.copy_penalty -= 1;
        if self.copy_penalty == 0 {
            self.copy_penalty_start += 1;
        }
    }

    #[inline(always)]
    pub fn update(&mut self, incompressible: bool) {
        if incompressible {
            if self.previous_incompressible {
                self.copy_penalty = self.copy_penalty_start;
            }
            self.previous_incompressible = true;
        } else {
            self.previous_incompressible = false;
        }
    }
}

pub trait Codec: QuadEncoder + QuadDecoder {
    fn encode_block_size(&self) -> usize;
    fn decode_unit_items(&self) -> usize;

    #[inline(always)]
    fn encode_block(&mut self, block: &[u8], out_buffer: &mut WriteBuffer, signature: &mut WriteSignature, protection_state: &mut ProtectionState) {
        if protection_state.revert_to_copy() {
            out_buffer.push(&block);
            protection_state.decay();
        } else {
            let mark = out_buffer.index;
            for bytes_16 in block.chunks(BYTE_SIZE_U128) {
                match <&[u8] as TryInto<[u8; BYTE_SIZE_U128]>>::try_into(bytes_16) {
                    Ok(array) => {
                        let value_u128 = u128::from_ne_bytes(array);

                        #[cfg(target_endian = "little")]
                        {
                            self.encode_quad((value_u128 & 0xffffffff) as u32, out_buffer, signature);
                            self.encode_quad(((value_u128 >> 32) & 0xffffffff) as u32, out_buffer, signature);
                            self.encode_quad(((value_u128 >> 64) & 0xffffffff) as u32, out_buffer, signature);
                            self.encode_quad((value_u128 >> 96) as u32, out_buffer, signature);
                        }

                        #[cfg(target_endian = "big")]
                        {
                            self.encode_quad((value_u128 >> 96) as u32, output, out_buffer, signature);
                            self.encode_quad(((value_u128 >> 64) & 0xffffffff) as u32, out_buffer, signature);
                            self.encode_quad(((value_u128 >> 32) & 0xffffffff) as u32, out_buffer, signature);
                            self.encode_quad((value_u128 & 0xffffffff) as u32, out_buffer, signature);
                        }
                    }
                    Err(_error) => {
                        // Less than 16 bytes left
                        for bytes_4 in block.chunks(BYTE_SIZE_U32) {
                            match <&[u8] as TryInto<[u8; BYTE_SIZE_U32]>>::try_into(bytes_4) {
                                Ok(array) => {
                                    self.encode_quad(u32::from_ne_bytes(array), out_buffer, signature);
                                }
                                Err(_error) => {
                                    out_buffer.push(bytes_4);
                                }
                            }
                        }
                    }
                }
            }
            out_buffer.ink(signature);
            protection_state.update(out_buffer.index - mark >= self.encode_block_size());
        }
    }

    fn encode(&mut self, input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        let mut signature = WriteSignature::new(0);
        let mut out_buffer = WriteBuffer::new(output, BYTE_SIZE_U64);
        let mut protection_state = ProtectionState::new();
        for block in input.chunks(self.encode_block_size()) {
            self.encode_block(block, &mut out_buffer, &mut signature, &mut protection_state);
        }
        Ok(out_buffer.index)
    }

    fn decode(&mut self, input: &[u8], output: &mut [u8]) -> Result<usize, DecodeError> {
        let mut in_buffer = ReadBuffer::new(input);
        let mut out_buffer = WriteBuffer::new(output, 0);
        let mut protection_state = ProtectionState::new();

        while in_buffer.remaining() >= BYTE_SIZE_U64 + self.encode_block_size() {
            if protection_state.revert_to_copy() {
                out_buffer.push(in_buffer.read(self.encode_block_size()));
                protection_state.decay();
            } else {
                let mark = in_buffer.index;
                let mut signature = ReadSignature::new(in_buffer.read_u64());
                for _ in 0..self.encode_block_size() >> 3 {
                    self.decode_unit(&mut in_buffer, &mut signature, &mut out_buffer);
                }
                protection_state.update(in_buffer.index - mark >= self.encode_block_size());
            }
        }

        while in_buffer.remaining() > 0 {
            if protection_state.revert_to_copy() {
                let max_copy = usize::min(in_buffer.remaining(), self.encode_block_size());
                out_buffer.push(in_buffer.read(max_copy));
                protection_state.decay();
            } else {
                let mark = in_buffer.index;
                let mut signature = ReadSignature::new(in_buffer.read_u64());
                for _ in 0..self.decode_unit_items() {
                    if in_buffer.remaining() >= BYTE_SIZE_U32 << 1 {
                        self.decode_unit(&mut in_buffer, &mut signature, &mut out_buffer);
                    } else {
                        out_buffer.push(in_buffer.read(in_buffer.remaining()));
                        break;
                    }
                }
                protection_state.update(in_buffer.index - mark >= self.encode_block_size());
            }
        }

        Ok(out_buffer.index)
    }
}