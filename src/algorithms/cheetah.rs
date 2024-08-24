use crate::algorithms::cheetah::SignatureFlags::{Chunk, MapA, MapB, Predicted};
use crate::codec::Codec;
use crate::encode_buffer::EncodeBuffer;
use crate::errors::encode_error::EncodeError;
use crate::quad_encoder::QuadEncoder;
use crate::signature::Signature;
use std::mem::size_of;

pub(crate) const CHEETAH_HASH_BITS: usize = 16;
pub(crate) const CHEETAH_HASH_MULTIPLIER: u32 = 0x9D6EF916;
pub(crate) const BYTE_SIZE_U16: usize = size_of::<u16>();
pub(crate) const BYTE_SIZE_U32: usize = size_of::<u32>();
pub(crate) const BIT_SIZE_U32: usize = BYTE_SIZE_U32 << 3;
pub(crate) const BYTE_SIZE_U64: usize = size_of::<u64>();
pub(crate) const BYTE_SIZE_U128: usize = size_of::<u128>();


pub(crate) const FLAG_SIZE_BITS: u8 = 2;

enum SignatureFlags {
    Chunk = 0x3,
    MapB = 0x2,
    MapA = 0x1,
    Predicted = 0x0,
}

pub struct State {
    pub(crate) last_hash: u16,
    pub(crate) chunk_map: [ChunkMap; 1 << CHEETAH_HASH_BITS],
    pub(crate) prediction_map: [PredictionMap; 1 << CHEETAH_HASH_BITS],
}

#[derive(Copy, Clone)]
pub struct ChunkMap {
    pub(crate) chunk_a: u32,
    pub(crate) chunk_b: u32,
}

#[derive(Copy, Clone)]
pub struct PredictionMap {
    pub(crate) next: u32,
}

pub struct Cheetah {
    pub state: State,
}

impl Cheetah {
    pub fn encode(input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        let mut cheetah = Cheetah::new();
        cheetah.encode(input, output)
    }

    pub fn new() -> Self {
        Cheetah {
            state: State {
                chunk_map: [ChunkMap { chunk_a: 0, chunk_b: 0 }; 1 << CHEETAH_HASH_BITS],
                prediction_map: [PredictionMap { next: 0 }; 1 << CHEETAH_HASH_BITS],
                last_hash: 0,
            },
        }
    }
}

impl QuadEncoder for Cheetah {
    #[inline(always)]
    fn encode_quad(&mut self, quad: u32, buffer: &mut EncodeBuffer, signature: &mut Signature) {
        let hash_u16 = ((quad * CHEETAH_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHEETAH_HASH_BITS)) as u16;
        let predicted_chunk = &mut self.state.prediction_map[self.state.last_hash as usize].next;
        if *predicted_chunk != quad {
            let chunks = &mut self.state.chunk_map[hash_u16 as usize];
            let found_a = &mut chunks.chunk_a;
            if *found_a != quad {
                let found_b = &mut chunks.chunk_b;
                if *found_b != quad {
                    buffer.push(&quad.to_le_bytes());
                    if signature.push_bits(Chunk as u64, FLAG_SIZE_BITS) { buffer.ink(signature); } // Chunk
                } else {
                    buffer.push(&hash_u16.to_le_bytes());
                    if signature.push_bits(MapB as u64, FLAG_SIZE_BITS) { buffer.ink(signature); } // Map B
                }
                *found_b = *found_a;
                *found_a = quad;
            } else {
                buffer.push(&hash_u16.to_le_bytes());
                if signature.push_bits(MapA as u64, FLAG_SIZE_BITS) { buffer.ink(signature); } // Map A
            }
            *predicted_chunk = quad;
        } else {
            if signature.push_bits(Predicted as u64, FLAG_SIZE_BITS) { buffer.ink(signature); } // Predicted
        }
        self.state.last_hash = hash_u16;
    }
}

impl Codec for Cheetah {}