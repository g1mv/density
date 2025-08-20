// File: src/codec/quad_encoder.rs

use crate::io::write_buffer::WriteBuffer;
use crate::io::write_signature::WriteSignature;

pub trait QuadEncoder {
    /// 编码单个 u32 quad
    fn encode_quad(&mut self, quad: u32, out_buffer: &mut WriteBuffer, signature: &mut WriteSignature);

    /// 批量编码 u32 quads，支持 RVV 或标量实现
    fn encode_batch(&mut self, quads: &[u32], out_buffer: &mut WriteBuffer, signature: &mut WriteSignature);
}
