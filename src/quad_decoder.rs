use crate::decode_buffer::DecodeBuffer;
use crate::decode_signature::DecodeSignature;

pub trait QuadDecoder {
    fn decode_quads(&mut self, buffer: &mut DecodeBuffer, signature: &mut DecodeSignature) -> (u32, u32);
}