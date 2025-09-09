use crate::algorithms::PLAIN_FLAG;
use crate::codec::codec::Codec;
use crate::codec::decoder::Decoder;
use crate::codec::quad_encoder::QuadEncoder;
use crate::errors::decode_error::DecodeError;
use crate::errors::encode_error::EncodeError;
use crate::io::read_buffer::ReadBuffer;
use crate::io::read_signature::ReadSignature;
use crate::io::write_buffer::WriteBuffer;
use crate::io::write_signature::WriteSignature;
use crate::{BIT_SIZE_U16, BIT_SIZE_U32, BYTE_SIZE_U32};
use std::slice::{from_raw_parts, from_raw_parts_mut};

pub(crate) const CHAMELEON_HASH_BITS: usize = BIT_SIZE_U16;
pub(crate) const CHAMELEON_HASH_MULTIPLIER: u32 = 0x9D6EF916;

pub(crate) const FLAG_SIZE_BITS: u8 = 1;
pub(crate) const MAP_FLAG: u64 = 0x1;

pub(crate) const PLAIN_PLAIN_FLAGS: u64 = (PLAIN_FLAG << 1) | PLAIN_FLAG;
pub(crate) const MAP_PLAIN_FLAGS: u64 = (PLAIN_FLAG << 1) | MAP_FLAG;
pub(crate) const PLAIN_MAP_FLAGS: u64 = (MAP_FLAG << 1) | PLAIN_FLAG;
// pub(crate) const _MAP_MAP_FLAGS: u64 = (MAP_FLAG << 1) | MAP_FLAG;

pub(crate) const DECODE_TWIN_FLAG_MASK: u64 = 0x3;
pub(crate) const DECODE_TWIN_FLAG_MASK_BITS: u8 = 2;
pub(crate) const DECODE_FLAG_MASK: u64 = 0x1;
pub(crate) const DECODE_FLAG_MASK_BITS: u8 = 1;

pub struct State {
    pub(crate) chunk_map: Vec<u32>,
}

pub struct Chameleon {
    pub state: State,
}

impl Chameleon {
    pub fn new() -> Self {
        Chameleon {
            state: State { chunk_map: vec![0; 1 << CHAMELEON_HASH_BITS] },
        }
    }

    pub fn encode(input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
        {
            // 检测是否支持 RVV，如果支持且数据量足够则使用 RVV 优化版本
            if Self::is_rvv_available() && input.len() >= 128 {
                return Self::encode_rvv(input, output);
            }
        }
        
        // 回退到标准实现
        let mut chameleon = Chameleon::new();
        chameleon.encode(input, output)
    }

    pub fn decode(input: &[u8], output: &mut [u8]) -> Result<usize, DecodeError> {
        #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
        {
            // 检测是否支持 RVV，如果支持且数据量足够则使用 RVV 优化版本
            if Self::is_rvv_available() && input.len() >= 64 {
                return Self::decode_rvv(input, output);
            }
        }
        
        // 回退到标准实现
        let mut chameleon = Chameleon::new();
        chameleon.decode(input, output)
    }

    #[inline(always)]
    fn decode_plain(&mut self, in_buffer: &mut ReadBuffer) -> u32 {
        let quad = in_buffer.read_u32_le();
        let hash = (quad.wrapping_mul(CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
        self.state.chunk_map[hash as usize] = quad;
        quad
    }

    #[inline(always)]
    fn decode_map(&mut self, in_buffer: &mut ReadBuffer) -> u32 {
        let hash = in_buffer.read_u16_le();
        let quad = self.state.chunk_map[hash as usize];
        quad
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn chameleon_encode(input: *const u8, input_size: usize, output: *mut u8, output_size: usize) -> usize {
        unsafe { Self::encode(from_raw_parts(input, input_size), from_raw_parts_mut(output, output_size)).unwrap_or(0) }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn chameleon_decode(input: *const u8, input_size: usize, output: *mut u8, output_size: usize) -> usize {
        unsafe { Self::decode(from_raw_parts(input, input_size), from_raw_parts_mut(output, output_size)).unwrap_or(0) }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn chameleon_safe_encode_buffer_size(size: usize) -> usize {
        Self::safe_encode_buffer_size(size)
    }

    // ==== RVV Optimization Implementation ====
    
    // ==== RVV Optimization Implementation ====
    
    /// Detect if RVV is supported
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    #[inline(always)]
    fn is_rvv_available() -> bool {
        // Runtime detection of RVV support
        Self::detect_rvv_capability()
    }
    
    #[cfg(not(all(target_arch = "riscv64", target_feature = "v")))]
    #[inline(always)]
    fn is_rvv_available() -> bool {
        false
    }
    
    /// 检测 RVV 能力
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    #[inline(always)]
    fn detect_rvv_capability() -> bool {
        unsafe {
            use core::arch::riscv64::*;
            // 检测 VLEN 是否足够支持批量处理
            let vl = vsetvli(8, VtypeBuilder::e32m1());
            vl >= 4  // 至少需要能处理 4 个 u32
        }
    }
    
    /// RVV 优化的编码实现
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    fn encode_rvv(input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        let mut chameleon = Chameleon::new();
        let mut in_buffer = ReadBuffer::new(input)?;
        let mut out_buffer = WriteBuffer::new(output);
        let mut protection_state = ProtectionState::new();

        // 使用 RVV 优化的编码处理
        chameleon.encode_process_rvv(&mut in_buffer, &mut out_buffer, &mut protection_state)?;
        
        Ok(out_buffer.index)
    }
    
    /// RVV 优化的解码实现
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    fn decode_rvv(input: &[u8], output: &mut [u8]) -> Result<usize, DecodeError> {
        let mut chameleon = Chameleon::new();
        let mut in_buffer = ReadBuffer::new(input)?;
        let mut out_buffer = WriteBuffer::new(output);
        let mut protection_state = ProtectionState::new();

        // 使用 RVV 优化的解码处理
        chameleon.decode_process_rvv(&mut in_buffer, &mut out_buffer, &mut protection_state)?;
        
        Ok(out_buffer.index)
    }
    
    /// RVV 优化的编码处理流程
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    fn encode_process_rvv(&mut self, 
                         in_buffer: &mut ReadBuffer, 
                         out_buffer: &mut WriteBuffer, 
                         protection_state: &mut ProtectionState) -> Result<(), EncodeError> {
        
        let iterations = Self::block_size() / Self::decode_unit_size();
        
        while in_buffer.remaining() > 0 {
            if protection_state.revert_to_copy() {
                // 保护状态：直接复制
                if in_buffer.remaining() > Self::block_size() {
                    out_buffer.push(in_buffer.read(Self::block_size()));
                } else {
                    out_buffer.push(in_buffer.read(in_buffer.remaining()));
                    break;
                }
                protection_state.decay();
            } else {
                // 正常编码
                let mark = out_buffer.index;
                let mut signature = WriteSignature::new();
                
                // 准备批量数据
                let available_bytes = in_buffer.remaining().min(Self::block_size());
                let quad_count = available_bytes / BYTE_SIZE_U32;
                
                if quad_count >= 8 {
                    // 有足够数据进行向量化处理
                    let mut quads = Vec::with_capacity(quad_count);
                    for _ in 0..quad_count {
                        if in_buffer.remaining() >= BYTE_SIZE_U32 {
                            quads.push(in_buffer.read_u32_le());
                        }
                    }
                    
                    // 使用 RVV 批量处理
                    self.encode_batch_rvv(&quads, out_buffer, &mut signature);
                } else {
                    // 数据太少，使用标量处理
                    for _ in 0..iterations {
                        if in_buffer.remaining() >= BYTE_SIZE_U32 {
                            let quad = in_buffer.read_u32_le();
                            self.encode_quad(quad, out_buffer, &mut signature);
                        } else if in_buffer.remaining() > 0 {
                            // 处理不足 4 字节的数据
                            let remaining_bytes = in_buffer.read(in_buffer.remaining());
                            signature.push_bits(PLAIN_FLAG, FLAG_SIZE_BITS);
                            out_buffer.push(remaining_bytes);
                            break;
                        }
                    }
                }
                
                Self::write_signature(out_buffer, &mut signature);
                protection_state.update(out_buffer.index - mark >= Self::block_size());
            }
        }
        
        Ok(())
    }
    
    /// 向量化批量编码核心循环
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    #[inline(always)]
    fn encode_batch_rvv(&mut self, 
                        quads: &[u32], 
                        out_buffer: &mut WriteBuffer, 
                        signature: &mut WriteSignature) -> usize {
        let len = quads.len();
        let mut processed = 0;

        // 处理向量长度的批次
        while processed + 8 <= len {
            unsafe {
                use core::arch::riscv64::*;
                
                // 设置向量长度为 8 个元素 (32 字节)
                let vl = vsetvli(8, VtypeBuilder::e32m1());
                
                if vl < 8 {
                    // VLEN 太小，回退到标量处理
                    break;
                }

                // 加载 8 个 u32 数据
                let quads_vec = vle32_v_u32m1(quads.as_ptr().add(processed), vl);
                
                // Vectorized hash calculation: hash = (quad * MULTIPLIER) >> (32 - HASH_BITS)
                let multiplier_vec = vmv_v_x_u32m1(CHAMELEON_HASH_MULTIPLIER, vl);
                let hash_temp = vmul_vv_u32m1(quads_vec, multiplier_vec, vl);
                let shift_amount = 32 - CHAMELEON_HASH_BITS;
                let hashes = vsrl_vx_u32m1(hash_temp, shift_amount as usize, vl);
                
                // Convert hash values to index array
                let mut hash_indices = [0u32; 8];
                vse32_v_u32m1(hash_indices.as_mut_ptr(), hashes, vl);
                
                // Batch check conflicts and processing
                let mut conflicts = false;
                let mut quad_array = [0u32; 8];
                vse32_v_u32m1(quad_array.as_mut_ptr(), quads_vec, vl);
                
                // Check hash conflicts - this part needs scalar processing to ensure correctness
                for i in 0..vl {
                    let hash_idx = (hash_indices[i] & ((1 << CHAMELEON_HASH_BITS) - 1)) as usize;
                    let quad = quad_array[i];
                    
                    // Check if conflicts with existing entries
                    if self.state.chunk_map[hash_idx] != 0 && self.state.chunk_map[hash_idx] != quad {
                        conflicts = true;
                        break;
                    }
                }
                
                if conflicts {
                    // Has conflicts, fallback to scalar processing for this batch
                    break;
                } else {
                    // No conflicts, batch processing
                    for i in 0..vl {
                        let hash_idx = (hash_indices[i] & ((1 << CHAMELEON_HASH_BITS) - 1)) as usize;
                        let quad = quad_array[i];
                        
                        if self.state.chunk_map[hash_idx] == quad && quad != 0 {
                            // Match: output compressed flag
                            signature.push_bits(MAP_FLAG, FLAG_SIZE_BITS);
                            out_buffer.push(&(hash_idx as u16).to_le_bytes());
                        } else {
                            // No match: output original data and update dictionary
                            signature.push_bits(PLAIN_FLAG, FLAG_SIZE_BITS);
                            out_buffer.push(&quad.to_le_bytes());
                            self.state.chunk_map[hash_idx] = quad;
                        }
                    }
                    processed += vl;
                }
            }
        }
        
        // 处理剩余的数据（标量处理）
        while processed < len {
            self.encode_quad_scalar(quads[processed], out_buffer, signature);
            processed += 1;
        }
        
        processed
    }
    
    /// 标量版本的 encode_quad（用于回退和剩余数据处理）
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    #[inline(always)]
    fn encode_quad_scalar(&mut self, quad: u32, out_buffer: &mut WriteBuffer, signature: &mut WriteSignature) {
        let hash = ((quad.wrapping_mul(CHAMELEON_HASH_MULTIPLIER)) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as usize;
        let hash_idx = hash & ((1 << CHAMELEON_HASH_BITS) - 1);
        
        if self.state.chunk_map[hash_idx] == quad && quad != 0 {
            // 匹配：压缩
            signature.push_bits(MAP_FLAG, FLAG_SIZE_BITS);
            out_buffer.push(&(hash_idx as u16).to_le_bytes());
        } else {
            // 不匹配：输出原始数据
            signature.push_bits(PLAIN_FLAG, FLAG_SIZE_BITS);
            out_buffer.push(&quad.to_le_bytes());
            self.state.chunk_map[hash_idx] = quad;
        }
    }
    
    /// RVV 优化的解码处理流程
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    fn decode_process_rvv(&mut self, 
                         in_buffer: &mut ReadBuffer, 
                         out_buffer: &mut WriteBuffer, 
                         protection_state: &mut ProtectionState) -> Result<(), DecodeError> {
        
        let iterations = Self::block_size() / Self::decode_unit_size();
        
        while in_buffer.remaining() > 0 {
            if protection_state.revert_to_copy() {
                // 保护状态：直接复制
                if in_buffer.remaining() > Self::block_size() {
                    out_buffer.push(in_buffer.read(Self::block_size()));
                } else {
                    out_buffer.push(in_buffer.read(in_buffer.remaining()));
                    break;
                }
                protection_state.decay();
            } else {
                // 正常解码
                let mark = in_buffer.index;
                let mut signature = Self::read_signature(in_buffer);
                
                for _ in 0..iterations {
                    if in_buffer.remaining() >= Self::decode_unit_size() {
                        let quad = self.decode_unit_rvv(in_buffer, &mut signature);
                        out_buffer.push(&quad.to_le_bytes());
                    } else {
                        if self.decode_partial_unit_rvv(in_buffer, &mut signature, out_buffer) {
                            break;
                        }
                    }
                }
                
                protection_state.update(in_buffer.index - mark >= Self::block_size());
            }
        }
        
        Ok(())
    }

    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    #[inline(always)]
    fn decode_unit_rvv(&mut self, in_buffer: &mut ReadBuffer, signature: &mut ReadSignature) -> u32 {
        // 对于 Chameleon，解码逻辑相对简单，直接使用原有逻辑
        if signature.read_bits(DECODE_FLAG_MASK, DECODE_FLAG_MASK_BITS) == PLAIN_FLAG {
            self.decode_plain(in_buffer)
        } else {
            self.decode_map(in_buffer)
        }
    }

    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    #[inline(always)]
    fn decode_partial_unit_rvv(&mut self, 
                              in_buffer: &mut ReadBuffer, 
                              signature: &mut ReadSignature, 
                              out_buffer: &mut WriteBuffer) -> bool {
        // 使用原有的 decode_partial_unit 逻辑
        self.decode_partial_unit(in_buffer, signature, out_buffer)
    }
}

impl QuadEncoder for Chameleon {
    #[inline(always)]
    fn encode_quad(&mut self, quad: u32, out_buffer: &mut WriteBuffer, signature: &mut WriteSignature) {
        let hash_u16 = (quad.wrapping_mul(CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
        let dictionary_value = &mut self.state.chunk_map[hash_u16 as usize];
        if *dictionary_value != quad {
            signature.push_bits(PLAIN_FLAG, FLAG_SIZE_BITS);
            out_buffer.push(&quad.to_le_bytes());

            *dictionary_value = quad;
        } else {
            signature.push_bits(MAP_FLAG, FLAG_SIZE_BITS);
            out_buffer.push(&hash_u16.to_le_bytes());
        }
    }
}

impl Decoder for Chameleon {
    #[inline(always)]
    fn decode_unit(&mut self, in_buffer: &mut ReadBuffer, signature: &mut ReadSignature, out_buffer: &mut WriteBuffer) {
        let (quad_a, quad_b) = match signature.read_bits(DECODE_TWIN_FLAG_MASK, DECODE_TWIN_FLAG_MASK_BITS) {
            PLAIN_PLAIN_FLAGS => { (self.decode_plain(in_buffer), self.decode_plain(in_buffer)) }
            MAP_PLAIN_FLAGS => { (self.decode_map(in_buffer), self.decode_plain(in_buffer)) }
            PLAIN_MAP_FLAGS => { (self.decode_plain(in_buffer), self.decode_map(in_buffer)) }
            _ => { (self.decode_map(in_buffer), self.decode_map(in_buffer)) }
        };
        out_buffer.push(&quad_a.to_le_bytes());
        out_buffer.push(&quad_b.to_le_bytes());
    }

    #[inline(always)]
    fn decode_partial_unit(&mut self, in_buffer: &mut ReadBuffer, signature: &mut ReadSignature, out_buffer: &mut WriteBuffer) -> bool {
        for _ in 0..Self::decode_unit_size() / BYTE_SIZE_U32 {
            let quad = match signature.read_bits(DECODE_FLAG_MASK, DECODE_FLAG_MASK_BITS) {
                PLAIN_FLAG => {
                    match in_buffer.remaining() {
                        0 => { return true; }
                        1..=3 => {
                            out_buffer.push(in_buffer.read(in_buffer.remaining()));
                            return true;
                        }
                        _ => { self.decode_plain(in_buffer) }
                    }
                }
                _ => { self.decode_map(in_buffer) }
            };
            out_buffer.push(&quad.to_le_bytes());
        }
        false
    }
}

impl Codec for Chameleon {
    #[inline(always)]
    fn block_size() -> usize { BYTE_SIZE_U32 * (Self::signature_significant_bytes() << 3) }

    #[inline(always)]
    fn decode_unit_size() -> usize { 8 }

    #[inline(always)]
    fn signature_significant_bytes() -> usize { 8 }

    fn clear_state(&mut self) {
        self.state.chunk_map.fill(0);
    }
}
