use crate::codec::decoder::Decoder;
use crate::codec::protection_state::ProtectionState;
use crate::codec::quad_encoder::QuadEncoder;
use crate::errors::decode_error::DecodeError;
use crate::errors::encode_error::EncodeError;
use crate::io::read_buffer::ReadBuffer;
use crate::io::read_signature::ReadSignature;
use crate::io::write_buffer::WriteBuffer;
use crate::io::write_signature::WriteSignature;
use crate::{BYTE_SIZE_SIGNATURE, BYTE_SIZE_U128, BYTE_SIZE_U32, BYTE_SIZE_U64};

pub trait Codec: QuadEncoder + Decoder {
    fn block_size(&self) -> usize;
    fn decode_units_per_block(&self) -> usize;

    #[inline(always)]
    fn encode_block(&mut self, block: &[u8], out_buffer: &mut WriteBuffer, signature: &mut WriteSignature, protection_state: &mut ProtectionState) {
        /*if protection_state.revert_to_copy() {
            out_buffer.push(&block);
            protection_state.decay();
        } else {
            let mark = out_buffer.index;*/
        for sub_block in block.chunks(BYTE_SIZE_U128) {
            match <&[u8] as TryInto<[u8; BYTE_SIZE_U128]>>::try_into(sub_block) {
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
                    for bytes in sub_block.chunks(BYTE_SIZE_U32) {
                        match <&[u8] as TryInto<[u8; BYTE_SIZE_U32]>>::try_into(bytes) {
                            Ok(array) => {
                                self.encode_quad(u32::from_ne_bytes(array), out_buffer, signature);
                            }
                            Err(_error) => {
                                // Implicit signature plain flag
                                out_buffer.push(bytes);
                            }
                        }
                    }
                }
            }
        }
        out_buffer.ink(signature);
        /*protection_state.update(out_buffer.index - mark >= self.block_size());
    }*/
    }

    fn encode(&mut self, input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        let mut signature = WriteSignature::new(0);
        let mut out_buffer = WriteBuffer::new(output, BYTE_SIZE_U64);
        let mut protection_state = ProtectionState::new();
        for block in input.chunks(self.block_size()) {
            self.encode_block(block, &mut out_buffer, &mut signature, &mut protection_state);
        }
        Ok(out_buffer.index)
    }

    fn decode(&mut self, input: &[u8], output: &mut [u8]) -> Result<usize, DecodeError> {
        let mut in_buffer = ReadBuffer::new(input);
        let mut out_buffer = WriteBuffer::new(output, 0);
        let mut protection_state = ProtectionState::new();

        while in_buffer.remaining() >= BYTE_SIZE_SIGNATURE + self.block_size() {
            /*if protection_state.revert_to_copy() {
                out_buffer.push(in_buffer.read(self.block_size()));
                protection_state.decay();
            } else {
                let mark = in_buffer.index;*/
            let mut signature = ReadSignature::new(in_buffer.read_u64_le());
            for _ in 0..self.decode_units_per_block() {
                self.decode_unit(&mut in_buffer, &mut signature, &mut out_buffer);
            }
            /*protection_state.update(in_buffer.index - mark >= self.block_size());
        }*/
        }

        'end: while in_buffer.remaining() > 0 {
            /*if protection_state.revert_to_copy() {
                let max_copy = usize::min(in_buffer.remaining(), self.block_size());
                out_buffer.push(in_buffer.read(max_copy));
                protection_state.decay();
            } else {
                let mark = in_buffer.index;*/
            let mut signature = ReadSignature::new(in_buffer.read_u64_le());
            for _ in 0..self.decode_units_per_block() {
                if in_buffer.remaining() >= BYTE_SIZE_U32 << 1 {
                    self.decode_unit(&mut in_buffer, &mut signature, &mut out_buffer);
                } else {
                    self.decode_partial_unit(&mut in_buffer, &mut signature, &mut out_buffer);
                    break 'end;
                }
            }
            /*protection_state.update(in_buffer.index - mark >= self.block_size());
        }*/
        }

        Ok(out_buffer.index)
    }
}