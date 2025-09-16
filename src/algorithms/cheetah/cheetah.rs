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

pub(crate) const CHEETAH_HASH_BITS: usize = BIT_SIZE_U16;
pub(crate) const CHEETAH_HASH_MULTIPLIER: u32 = 0x9D6EF916;


pub(crate) const FLAG_SIZE_BITS: u8 = 2;
pub(crate) const MAP_A_FLAG: u64 = 0x1;
pub(crate) const MAP_B_FLAG: u64 = 0x2;
pub(crate) const PREDICTED_FLAG: u64 = 0x3;
pub(crate) const DECODE_FLAG_MASK: u64 = 0x3;
pub(crate) const DECODE_FLAG_MASK_BITS: u8 = 2;

pub struct State {
    pub(crate) last_hash: u16,
    pub(crate) chunk_map: Vec<ChunkData>,
    pub(crate) prediction_map: Vec<PredictionData>,
}

#[derive(Copy, Clone)]
pub struct ChunkData {
    pub(crate) chunk_a: u32,
    pub(crate) chunk_b: u32,
}

#[derive(Copy, Clone)]
pub struct PredictionData {
    pub(crate) next: u32,
}

pub struct Cheetah {
    pub state: State,
}

impl Cheetah {
    pub fn new() -> Self {
        Cheetah {
            state: State {
                last_hash: 0,
                chunk_map: vec![ChunkData { chunk_a: 0, chunk_b: 0 }; 1 << CHEETAH_HASH_BITS],
                prediction_map: vec![PredictionData { next: 0 }; 1 << CHEETAH_HASH_BITS],
            },
        }
    }

    pub fn encode(input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
        {
            // Detect if RVV is supported, use RVV optimized version if supported and data size is sufficient
            if Self::is_rvv_available() && input.len() >= 128 {
                return Self::encode_rvv(input, output);
            }
        }
        
        // Fallback to standard implementation
        let mut cheetah = Cheetah::new();
        cheetah.encode(input, output)
    }

    pub fn decode(input: &[u8], output: &mut [u8]) -> Result<usize, DecodeError> {
        #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
        {
            // Detect if RVV is supported, use RVV optimized version if supported and data size is sufficient
            if Self::is_rvv_available() && input.len() >= 64 {
                return Self::decode_rvv(input, output);
            }
        }
        
        // Fallback to standard implementation
        let mut cheetah = Cheetah::new();
        cheetah.decode(input, output)
    }

    #[inline(always)]
    fn decode_plain(&mut self, in_buffer: &mut ReadBuffer) -> (u16, u32) {
        let quad = in_buffer.read_u32_le();
        let hash = (quad.wrapping_mul(CHEETAH_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHEETAH_HASH_BITS)) as u16;
        let chunk_data = &mut self.state.chunk_map[hash as usize];
        chunk_data.chunk_b = chunk_data.chunk_a;
        chunk_data.chunk_a = quad;
        self.state.prediction_map[self.state.last_hash as usize] = PredictionData { next: quad };
        (hash, quad)
    }

    #[inline(always)]
    fn decode_map_a(&mut self, in_buffer: &mut ReadBuffer) -> (u16, u32) {
        let hash = in_buffer.read_u16_le();
        let chunk_data = &mut self.state.chunk_map[hash as usize];
        let quad = chunk_data.chunk_a;
        self.state.prediction_map[self.state.last_hash as usize] = PredictionData { next: quad };
        (hash, quad)
    }

    #[inline(always)]
    fn decode_map_b(&mut self, in_buffer: &mut ReadBuffer) -> (u16, u32) {
        let hash = in_buffer.read_u16_le();
        let chunk_data = &mut self.state.chunk_map[hash as usize];
        let quad = chunk_data.chunk_b;
        chunk_data.chunk_b = chunk_data.chunk_a;
        chunk_data.chunk_a = quad;
        self.state.prediction_map[self.state.last_hash as usize] = PredictionData { next: quad };
        (hash, quad)
    }

    #[inline(always)]
    fn decode_predicted(&mut self) -> (u16, u32) {
        let quad = self.state.prediction_map[self.state.last_hash as usize].next;
        let hash = (quad.wrapping_mul(CHEETAH_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHEETAH_HASH_BITS)) as u16;
        (hash, quad)
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn cheetah_encode(input: *const u8, input_size: usize, output: *mut u8, output_size: usize) -> usize {
        unsafe { Self::encode(from_raw_parts(input, input_size), from_raw_parts_mut(output, output_size)).unwrap_or(0) }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn cheetah_decode(input: *const u8, input_size: usize, output: *mut u8, output_size: usize) -> usize {
        unsafe { Self::decode(from_raw_parts(input, input_size), from_raw_parts_mut(output, output_size)).unwrap_or(0) }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn cheetah_safe_encode_buffer_size(size: usize) -> usize {
        Self::safe_encode_buffer_size(size)
    }

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
    
    /// Detect RVV capability
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    #[inline(always)]
    fn detect_rvv_capability() -> bool {
        unsafe {
            use core::arch::riscv64::*;
            // Detect if VLEN is sufficient to support batch processing
            let vl = vsetvli(4, VtypeBuilder::e32m1());
            vl >= 4  // Cheetah's prediction logic is more complex, needs smaller batches
        }
    }
    
    /// RVV optimized encoding implementation
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    fn encode_rvv(input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        let mut cheetah = Cheetah::new();
        let mut in_buffer = ReadBuffer::new(input)?;
        let mut out_buffer = WriteBuffer::new(output);
        let mut protection_state = ProtectionState::new();

        // Use RVV optimized encoding processing
        cheetah.encode_process_rvv(&mut in_buffer, &mut out_buffer, &mut protection_state)?;
        
        Ok(out_buffer.index)
    }
    
    /// RVV optimized decoding implementation
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    fn decode_rvv(input: &[u8], output: &mut [u8]) -> Result<usize, DecodeError> {
        let mut cheetah = Cheetah::new();
        let mut in_buffer = ReadBuffer::new(input)?;
        let mut out_buffer = WriteBuffer::new(output);
        let mut protection_state = ProtectionState::new();

        // Use RVV optimized decoding processing
        cheetah.decode_process_rvv(&mut in_buffer, &mut out_buffer, &mut protection_state)?;
        
        Ok(out_buffer.index)
    }
    
    /// RVV optimized encoding processing flow
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    fn encode_process_rvv(&mut self, 
                         in_buffer: &mut ReadBuffer, 
                         out_buffer: &mut WriteBuffer, 
                         protection_state: &mut ProtectionState) -> Result<(), EncodeError> {
        
        let iterations = Self::block_size() / Self::decode_unit_size();
        
        while in_buffer.remaining() > 0 {
            if protection_state.revert_to_copy() {
                if in_buffer.remaining() > Self::block_size() {
                    out_buffer.push(in_buffer.read(Self::block_size()));
                } else {
                    out_buffer.push(in_buffer.read(in_buffer.remaining()));
                    break;
                }
                protection_state.decay();
            } else {
                let mark = out_buffer.index;
                let mut signature = WriteSignature::new();
                
                let available_bytes = in_buffer.remaining().min(Self::block_size());
                let quad_count = available_bytes / BYTE_SIZE_U32;
                
                if quad_count >= 4 {
                    // Cheetah's prediction logic is more complex, use smaller batches
                    let mut quads = Vec::with_capacity(quad_count);
                    for _ in 0..quad_count {
                        if in_buffer.remaining() >= BYTE_SIZE_U32 {
                            quads.push(in_buffer.read_u32_le());
                        }
                    }
                    
                    self.encode_batch_cheetah_rvv(&quads, out_buffer, &mut signature);
                } else {
                    // Insufficient data, use scalar processing
                    for _ in 0..iterations {
                        if in_buffer.remaining() >= BYTE_SIZE_U32 {
                            let quad = in_buffer.read_u32_le();
                            self.encode_quad(quad, out_buffer, &mut signature);
                        } else if in_buffer.remaining() > 0 {
                            let remaining_bytes = in_buffer.read(in_buffer.remaining());
                            signature.push_bits(PREDICTION_FLAG, FLAG_SIZE_BITS);
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
    
    /// Vectorized Cheetah prediction processing
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    #[inline(always)]
    fn encode_batch_cheetah_rvv(&mut self, 
                               quads: &[u32], 
                               out_buffer: &mut WriteBuffer, 
                               signature: &mut WriteSignature) -> usize {
        let len = quads.len();
        let mut processed = 0;

        // Cheetah's prediction logic is more complex, use smaller batch sizes
        while processed + 4 <= len {
            unsafe {
                use core::arch::riscv64::*;
                
                let vl = vsetvli(4, VtypeBuilder::e32m1());
                
                if vl < 4 {
                    break;
                }

                // Load 4 u32 data
                let quads_vec = vle32_v_u32m1(quads.as_ptr().add(processed), vl);
                
                // Vectorized hash calculation
                let multiplier_vec = vmv_v_x_u32m1(CHEETAH_HASH_MULTIPLIER, vl);
                let hash_temp = vmul_vv_u32m1(quads_vec, multiplier_vec, vl);
                let shift_amount = 32 - CHEETAH_HASH_BITS;
                let hashes = vsrl_vx_u32m1(hash_temp, shift_amount as usize, vl);
                
                let mut hash_indices = [0u32; 4];
                let mut quad_array = [0u32; 4];
                vse32_v_u32m1(hash_indices.as_mut_ptr(), hashes, vl);
                vse32_v_u32m1(quad_array.as_mut_ptr(), quads_vec, vl);
                
                // Check predictions and conflicts
                let mut has_conflicts = false;
                for i in 0..vl {
                    let hash_idx = (hash_indices[i] & ((1 << CHEETAH_HASH_BITS) - 1)) as usize;
                    let quad = quad_array[i];
                    
                    // Cheetah specific prediction logic check
                    let chunk_data = &self.state.chunk_map[hash_idx];
                    let prediction = self.state.prediction_map[self.state.last_hash as usize].next;
                    
                    // Check if complex prediction logic is suitable for batch processing
                    if chunk_data.chunk_a != 0 && prediction != 0 {
                        // Has complex state, may need precise sequential processing
                        has_conflicts = true;
                        break;
                    }
                }
                
                if has_conflicts {
                    // Fallback to scalar processing
                    break;
                } else {
                    // Batch processing (simplified Cheetah logic)
                    for i in 0..vl {
                        let hash_idx = (hash_indices[i] & ((1 << CHEETAH_HASH_BITS) - 1)) as usize;
                        let quad = quad_array[i];
                        
                        self.encode_quad_cheetah_scalar(hash_idx, quad, out_buffer, signature);
                    }
                    processed += vl;
                }
            }
        }
        
        // Process remaining data
        while processed < len {
            let quad = quads[processed];
            let hash = ((quad.wrapping_mul(CHEETAH_HASH_MULTIPLIER)) >> (BIT_SIZE_U32 - CHEETAH_HASH_BITS)) as usize;
            let hash_idx = hash & ((1 << CHEETAH_HASH_BITS) - 1);
            self.encode_quad_cheetah_scalar(hash_idx, quad, out_buffer, signature);
            processed += 1;
        }
        
        processed
    }
    
    /// Cheetah scalar encoding (used for fallback)
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    #[inline(always)]
    fn encode_quad_cheetah_scalar(&mut self, 
                                 hash_idx: usize, 
                                 quad: u32, 
                                 out_buffer: &mut WriteBuffer, 
                                 signature: &mut WriteSignature) {
        // Use original encode_quad logic
        self.encode_quad(quad, out_buffer, signature);
    }
    
    /// RVV optimized decoding processing flow
    #[cfg(all(target_arch = "riscv64", target_feature = "v"))]
    fn decode_process_rvv(&mut self, 
                         in_buffer: &mut ReadBuffer, 
                         out_buffer: &mut WriteBuffer, 
                         protection_state: &mut ProtectionState) -> Result<(), DecodeError> {
        
        let iterations = Self::block_size() / Self::decode_unit_size();
        
        while in_buffer.remaining() > 0 {
            if protection_state.revert_to_copy() {
                if in_buffer.remaining() > Self::block_size() {
                    out_buffer.push(in_buffer.read(Self::block_size()));
                } else {
                    out_buffer.push(in_buffer.read(in_buffer.remaining()));
                    break;
                }
                protection_state.decay();
            } else {
                let mark = in_buffer.index;
                let mut signature = Self::read_signature(in_buffer);
                
                for _ in 0..iterations {
                    if in_buffer.remaining() >= Self::decode_unit_size() {
                        self.decode_unit(in_buffer, &mut signature, out_buffer);
                    } else {
                        if self.decode_partial_unit(in_buffer, &mut signature, out_buffer) {
                            break;
                        }
                    }
                }
                
                protection_state.update(in_buffer.index - mark >= Self::block_size());
            }
        }
        
        Ok(())
    }
}

impl QuadEncoder for Cheetah {
    #[inline(always)]
    fn encode_quad(&mut self, quad: u32, out_buffer: &mut WriteBuffer, signature: &mut WriteSignature) {
        let hash_u16 = (quad.wrapping_mul(CHEETAH_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHEETAH_HASH_BITS)) as u16;
        let predicted_chunk = &mut self.state.prediction_map[self.state.last_hash as usize].next;
        if *predicted_chunk != quad {
            let chunk_data = &mut self.state.chunk_map[hash_u16 as usize];
            let offer_a = &mut chunk_data.chunk_a;
            if *offer_a != quad {
                let offer_b = &mut chunk_data.chunk_b;
                if *offer_b != quad {
                    signature.push_bits(PLAIN_FLAG, FLAG_SIZE_BITS); // Chunk
                    out_buffer.push(&quad.to_le_bytes());
                } else {
                    signature.push_bits(MAP_B_FLAG, FLAG_SIZE_BITS); // Map B
                    out_buffer.push(&hash_u16.to_le_bytes());
                }
                *offer_b = *offer_a;
                *offer_a = quad;
            } else {
                signature.push_bits(MAP_A_FLAG, FLAG_SIZE_BITS); // Map A
                out_buffer.push(&hash_u16.to_le_bytes());
            }
            *predicted_chunk = quad;
        } else {
            signature.push_bits(PREDICTED_FLAG, FLAG_SIZE_BITS); // Predicted
        }
        self.state.last_hash = hash_u16;
    }
}

impl Decoder for Cheetah {
    #[inline(always)]
    fn decode_unit(&mut self, in_buffer: &mut ReadBuffer, signature: &mut ReadSignature, out_buffer: &mut WriteBuffer) {
        let (hash, quad) = match signature.read_bits(DECODE_FLAG_MASK, DECODE_FLAG_MASK_BITS) {
            PLAIN_FLAG => { self.decode_plain(in_buffer) }
            MAP_A_FLAG => { self.decode_map_a(in_buffer) }
            MAP_B_FLAG => { self.decode_map_b(in_buffer) }
            _ => { self.decode_predicted() }
        };
        self.state.last_hash = hash;
        out_buffer.push(&quad.to_le_bytes());
    }

    #[inline(always)]
    fn decode_partial_unit(&mut self, in_buffer: &mut ReadBuffer, signature: &mut ReadSignature, out_buffer: &mut WriteBuffer) -> bool {
        let (hash, quad) = match signature.read_bits(DECODE_FLAG_MASK, DECODE_FLAG_MASK_BITS) {
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
            MAP_A_FLAG => { self.decode_map_a(in_buffer) }
            MAP_B_FLAG => { self.decode_map_b(in_buffer) }
            _ => { self.decode_predicted() }
        };
        self.state.last_hash = hash;
        out_buffer.push(&quad.to_le_bytes());
        false
    }
}

impl Codec for Cheetah {
    #[inline(always)]
    fn block_size() -> usize { BYTE_SIZE_U32 * (Self::signature_significant_bytes() << 3) / FLAG_SIZE_BITS as usize }

    #[inline(always)]
    fn decode_unit_size() -> usize { 4 }

    #[inline(always)]
    fn signature_significant_bytes() -> usize { 8 }

    fn clear_state(&mut self) {
        self.state.last_hash = 0;
        self.state.chunk_map.fill(ChunkData { chunk_a: 0, chunk_b: 0 });
        self.state.prediction_map.fill(PredictionData { next: 0 });
    }
}