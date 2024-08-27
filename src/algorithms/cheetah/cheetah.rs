use crate::codec::codec::Codec;
use crate::codec::quad_decoder::QuadDecoder;
use crate::codec::quad_encoder::QuadEncoder;
use crate::errors::decode_error::DecodeError;
use crate::errors::encode_error::EncodeError;
use crate::io::read_buffer::ReadBuffer;
use crate::io::read_signature::ReadSignature;
use crate::io::write_buffer::WriteBuffer;
use crate::io::write_signature::WriteSignature;
use crate::BIT_SIZE_U32;

pub(crate) const CHEETAH_HASH_BITS: usize = 16;
pub(crate) const CHEETAH_HASH_MULTIPLIER: u32 = 0x9D6EF916;


pub(crate) const FLAG_SIZE_BITS: u8 = 2;
pub(crate) const PREDICTED_FLAG: u64 = 0x0;
pub(crate) const MAP_A_FLAG: u64 = 0x1;
pub(crate) const MAP_B_FLAG: u64 = 0x2;
pub(crate) const PLAIN_FLAG: u64 = 0x3;
pub(crate) const DECODE_FLAG_MASK: u64 = 0x3;
pub(crate) const DECODE_FLAG_MASK_BITS: u8 = 2;

pub struct State {
    pub(crate) last_hash: u16,
    pub(crate) chunk_map: [ChunkData; 1 << CHEETAH_HASH_BITS],
    pub(crate) prediction_map: [PredictionData; 1 << CHEETAH_HASH_BITS],
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
    pub fn encode(input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        let mut cheetah = Cheetah::new();
        cheetah.encode(input, output)
    }

    pub fn decode(input: &[u8], output: &mut [u8]) -> Result<usize, DecodeError> {
        let mut cheetah = Cheetah::new();
        cheetah.decode(input, output)
    }

    pub fn new() -> Self {
        Cheetah {
            state: State {
                last_hash: 0,
                chunk_map: [ChunkData { chunk_a: 0, chunk_b: 0 }; 1 << CHEETAH_HASH_BITS],
                prediction_map: [PredictionData { next: 0 }; 1 << CHEETAH_HASH_BITS],
            },
        }
    }
}

impl QuadEncoder for Cheetah {
    #[inline(always)]
    fn encode_quad(&mut self, quad: u32, encode_buffer: &mut WriteBuffer, signature: &mut WriteSignature) {
        let hash_u16 = ((quad * CHEETAH_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHEETAH_HASH_BITS)) as u16;
        let predicted_chunk = &mut self.state.prediction_map[self.state.last_hash as usize].next;
        if *predicted_chunk != quad {
            let chunk_data = &mut self.state.chunk_map[hash_u16 as usize];
            let offer_a = &mut chunk_data.chunk_a;
            if *offer_a != quad {
                let offer_b = &mut chunk_data.chunk_b;
                if *offer_b != quad {
                    signature.push_bits(PLAIN_FLAG, FLAG_SIZE_BITS); // Chunk
                    encode_buffer.push(&quad.to_le_bytes());
                } else {
                    signature.push_bits(MAP_B_FLAG, FLAG_SIZE_BITS); // Map B
                    encode_buffer.push(&hash_u16.to_le_bytes());
                }
                *offer_b = *offer_a;
                *offer_a = quad;
            } else {
                signature.push_bits(MAP_A_FLAG, FLAG_SIZE_BITS); // Map A
                encode_buffer.push(&hash_u16.to_le_bytes());
            }
            *predicted_chunk = quad;
        } else {
            signature.push_bits(PREDICTED_FLAG, FLAG_SIZE_BITS); // Predicted
        }
        self.state.last_hash = hash_u16;
    }
}

impl QuadDecoder for Cheetah {
    #[inline(always)]
    fn decode_unit(&mut self, in_buffer: &mut ReadBuffer, signature: &mut ReadSignature, out_buffer: &mut WriteBuffer) {
        let (hash, quad) = match signature.read_bits(DECODE_FLAG_MASK, DECODE_FLAG_MASK_BITS) {
            PLAIN_FLAG => {
                let quad = in_buffer.read_u32_le();
                let hash = ((quad * CHEETAH_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHEETAH_HASH_BITS)) as u16;
                let chunk_data = &mut self.state.chunk_map[hash as usize];
                chunk_data.chunk_b = chunk_data.chunk_a;
                chunk_data.chunk_a = quad;
                self.state.prediction_map[self.state.last_hash as usize] = PredictionData { next: quad };
                (hash, quad)
            }
            MAP_A_FLAG => {
                let hash = in_buffer.read_u16_le();
                let chunk_data = &mut self.state.chunk_map[hash as usize];
                let quad = chunk_data.chunk_a;
                self.state.prediction_map[self.state.last_hash as usize] = PredictionData { next: quad };
                (hash, quad)
            }
            MAP_B_FLAG => {
                let hash = in_buffer.read_u16_le();
                let chunk_data = &mut self.state.chunk_map[hash as usize];
                let quad = chunk_data.chunk_b;
                chunk_data.chunk_b = chunk_data.chunk_a;
                chunk_data.chunk_a = quad;
                self.state.prediction_map[self.state.last_hash as usize] = PredictionData { next: quad };
                (hash, quad)
            }
            _ => {
                let quad = self.state.prediction_map[self.state.last_hash as usize].next;
                let hash = ((quad * CHEETAH_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHEETAH_HASH_BITS)) as u16;
                (hash, quad)
            }
        };
        self.state.last_hash = hash;
        out_buffer.push(&quad.to_le_bytes());
    }
}

impl Codec for Cheetah {
    #[inline(always)]
    fn block_size(&self) -> usize { 128 }

    #[inline(always)]
    fn decode_units_per_block(&self) -> usize { 32 }
}