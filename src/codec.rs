use crate::algorithms::cheetah::{BYTE_SIZE_U128, BYTE_SIZE_U64};
use crate::encode_buffer::EncodeBuffer;
use crate::errors::decode_error::DecodeError;
use crate::errors::encode_error::EncodeError;
use crate::quad_encoder::QuadEncoder;
use crate::signature::Signature;

pub trait Codec: QuadEncoder {
    fn encode(&mut self, input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        let start_index = 0;
        let mut signature = Signature::new(0);
        let mut out_buffer = EncodeBuffer::new(output, BYTE_SIZE_U64);
        for bytes in input.chunks(BYTE_SIZE_U128) {
            match <&[u8] as TryInto<[u8; BYTE_SIZE_U128]>>::try_into(bytes) {
                Ok(array) => {
                    let value_u128 = u128::from_ne_bytes(array);

                    #[cfg(target_endian = "little")]
                    {
                        self.encode_quad((value_u128 >> 96) as u32, &mut out_buffer, &mut signature);
                        self.encode_quad(((value_u128 >> 64) & 0xffffffff) as u32, &mut out_buffer, &mut signature);
                        self.encode_quad(((value_u128 >> 32) & 0xffffffff) as u32, &mut out_buffer, &mut signature);
                        self.encode_quad((value_u128 & 0xffffffff) as u32, &mut out_buffer, &mut signature);
                    }

                    #[cfg(target_endian = "big")]
                    {
                        self.encode_quad((value_u128 & 0xffffffff) as u32, &mut out_buffer, &mut signature);
                        self.encode_quad(((value_u128 >> 32) & 0xffffffff) as u32, &mut out_buffer, &mut signature);
                        self.encode_quad(((value_u128 >> 64) & 0xffffffff) as u32, &mut out_buffer, &mut signature);
                        self.encode_quad((value_u128 >> 96) as u32, output, &mut out_buffer, &mut signature);
                    }
                }
                Err(_error) => {
                    out_buffer.push(bytes);
                    println!("Ending");
                }
            }
        }
        if signature.shift > 0 {
            out_buffer.write_at(signature.pos, &signature.value.to_le_bytes());
        } else {
            // Signature reserved space has not been used
            out_buffer.rewind(BYTE_SIZE_U64);
        }
        // println!("{}", out_index - start_index);
        Ok(out_buffer.index - start_index)
    }

    fn decode(&mut self, input: &[u8], output: &mut [u8]) -> Result<(), DecodeError> {
        todo!()
    }
}