use crate::codec::Codec;
use crate::decode_buffer::DecodeBuffer;
use crate::decode_signature::DecodeSignature;
use crate::encode_buffer::EncodeBuffer;
use crate::encode_signature::EncodeSignature;
use crate::errors::decode_error::DecodeError;
use crate::errors::encode_error::EncodeError;
use crate::quad_decoder::QuadDecoder;
use crate::quad_encoder::QuadEncoder;
use crate::BIT_SIZE_U32;

pub(crate) const CHAMELEON_HASH_BITS: usize = 16;
pub(crate) const CHAMELEON_HASH_MULTIPLIER: u32 = 0x9D6EF916;

pub(crate) const FLAG_SIZE_BITS: u8 = 1;
pub(crate) const FLAG_MASK: u64 = 0x3;
pub(crate) const FLAG_MASK_BITS: u8 = 2;
pub(crate) const PLAIN_FLAG: u64 = 0x0;
pub(crate) const MAP_FLAG: u64 = 0x1;

pub(crate) const PLAIN_PLAIN_FLAGS: u64 = (PLAIN_FLAG << 1) | PLAIN_FLAG;
pub(crate) const PLAIN_MAP_FLAGS: u64 = (PLAIN_FLAG << 1) | MAP_FLAG;
pub(crate) const MAP_PLAIN_FLAGS: u64 = (MAP_FLAG << 1) | PLAIN_FLAG;
pub(crate) const MAP_MAP_FLAGS: u64 = (MAP_FLAG << 1) | MAP_FLAG;

// pub(crate) const PLAIN_PLAIN_PLAIN_PLAIN_FLAGS: u64 = (PLAIN_FLAG << 3) | (PLAIN_FLAG << 2) | (PLAIN_FLAG << 1) | PLAIN_FLAG;
// pub(crate) const PLAIN_PLAIN_PLAIN_MAP_FLAGS: u64 = (PLAIN_FLAG << 3) | (PLAIN_FLAG << 2) | (PLAIN_FLAG << 1) | MAP_FLAG;
// pub(crate) const PLAIN_PLAIN_MAP_PLAIN_FLAGS: u64 = (PLAIN_FLAG << 3) | (PLAIN_FLAG << 2) | (MAP_FLAG << 1) | PLAIN_FLAG;
// pub(crate) const PLAIN_PLAIN_MAP_MAP_FLAGS: u64 = (PLAIN_FLAG << 3) | (PLAIN_FLAG << 2) | (MAP_FLAG << 1) | MAP_FLAG;
// pub(crate) const PLAIN_MAP_PLAIN_PLAIN_FLAGS: u64 = (PLAIN_FLAG << 3) | (MAP_FLAG << 2) | (PLAIN_FLAG << 1) | PLAIN_FLAG;
// pub(crate) const PLAIN_MAP_PLAIN_MAP_FLAGS: u64 = (PLAIN_FLAG << 3) | (MAP_FLAG << 2) | (PLAIN_FLAG << 1) | MAP_FLAG;
// pub(crate) const PLAIN_MAP_MAP_PLAIN_FLAGS: u64 = (PLAIN_FLAG << 3) | (MAP_FLAG << 2) | (MAP_FLAG << 1) | PLAIN_FLAG;
// pub(crate) const PLAIN_MAP_MAP_MAP_FLAGS: u64 = (PLAIN_FLAG << 3) | (MAP_FLAG << 2) | (MAP_FLAG << 1) | MAP_FLAG;
// pub(crate) const MAP_PLAIN_PLAIN_PLAIN_FLAGS: u64 = (MAP_FLAG << 3) | (PLAIN_FLAG << 2) | (PLAIN_FLAG << 1) | PLAIN_FLAG;
// pub(crate) const MAP_PLAIN_PLAIN_MAP_FLAGS: u64 = (MAP_FLAG << 3) | (PLAIN_FLAG << 2) | (PLAIN_FLAG << 1) | MAP_FLAG;
// pub(crate) const MAP_PLAIN_MAP_PLAIN_FLAGS: u64 = (MAP_FLAG << 3) | (PLAIN_FLAG << 2) | (MAP_FLAG << 1) | PLAIN_FLAG;
// pub(crate) const MAP_PLAIN_MAP_MAP_FLAGS: u64 = (MAP_FLAG << 3) | (PLAIN_FLAG << 2) | (MAP_FLAG << 1) | MAP_FLAG;
// pub(crate) const MAP_MAP_PLAIN_PLAIN_FLAGS: u64 = (MAP_FLAG << 3) | (MAP_FLAG << 2) | (PLAIN_FLAG << 1) | PLAIN_FLAG;
// pub(crate) const MAP_MAP_PLAIN_MAP_FLAGS: u64 = (MAP_FLAG << 3) | (MAP_FLAG << 2) | (PLAIN_FLAG << 1) | MAP_FLAG;
// pub(crate) const MAP_MAP_MAP_PLAIN_FLAGS: u64 = (MAP_FLAG << 3) | (MAP_FLAG << 2) | (MAP_FLAG << 1) | PLAIN_FLAG;
// pub(crate) const MAP_MAP_MAP_MAP_FLAGS: u64 = (MAP_FLAG << 3) | (MAP_FLAG << 2) | (MAP_FLAG << 1) | MAP_FLAG;

pub struct State {
    pub(crate) chunk_map: [u32; 1 << CHAMELEON_HASH_BITS],
}

pub struct Chameleon {
    pub state: State,
}

impl Chameleon {
    pub fn new() -> Self {
        Chameleon {
            state: State { chunk_map: [0; 1 << CHAMELEON_HASH_BITS] },
        }
    }

    pub fn encode(input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        let mut chameleon = Chameleon::new();
        chameleon.encode(input, output)
    }

    pub fn decode(input: &[u8], output: &mut [u8]) -> Result<usize, DecodeError> {
        let mut chameleon = Chameleon::new();
        chameleon.decode(input, output)
    }
}

impl QuadEncoder for Chameleon {
    #[inline(always)]
    fn encode_quad(&mut self, quad: u32, encode_buffer: &mut EncodeBuffer, signature: &mut EncodeSignature) {
        let hash_u16 = ((quad.wrapping_mul(CHAMELEON_HASH_MULTIPLIER)) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
        let dictionary_value = &mut self.state.chunk_map[hash_u16 as usize];
        if *dictionary_value != quad {
            signature.push_bits(PLAIN_FLAG, FLAG_SIZE_BITS);
            encode_buffer.push(&quad.to_le_bytes());

            *dictionary_value = quad;
        } else {
            signature.push_bits(MAP_FLAG, FLAG_SIZE_BITS);
            encode_buffer.push(&hash_u16.to_le_bytes());
        }
    }
}

impl QuadDecoder for Chameleon {
    #[inline(always)]
    fn decode_quads(&mut self, buffer: &mut DecodeBuffer, signature: &mut DecodeSignature) -> (u32, u32) {
        match signature.read_bits(FLAG_MASK, FLAG_MASK_BITS) {
            // PLAIN_FLAG => {
            //     let quad_a = buffer.read_u32();
            //     let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     self.state.chunk_map[hash_a as usize] = quad_a;
            //     quad_a
            // }
            // _ => {
            //     let hash_a = buffer.read_u16();
            //     let quad_a = self.state.chunk_map[hash_a as usize];
            //     quad_a
            // }

            PLAIN_PLAIN_FLAGS => {
                let quad_a = buffer.read_u32();
                let quad_b = buffer.read_u32();
                let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
                let hash_b = ((quad_b * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
                self.state.chunk_map[hash_a as usize] = quad_a;
                self.state.chunk_map[hash_b as usize] = quad_b;
                (quad_a, quad_b)
            }
            PLAIN_MAP_FLAGS => {
                let quad_a = buffer.read_u32();
                let hash_b = buffer.read_u16();
                let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
                let quad_b = self.state.chunk_map[hash_b as usize];
                self.state.chunk_map[hash_a as usize] = quad_a;
                (quad_a, quad_b)
            }
            MAP_PLAIN_FLAGS => {
                let hash_a = buffer.read_u16();
                let quad_b = buffer.read_u32();
                let quad_a = self.state.chunk_map[hash_a as usize];
                let hash_b = ((quad_b * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
                self.state.chunk_map[hash_b as usize] = quad_b;
                (quad_a, quad_b)
            }
            _ => {
                let hash_a = buffer.read_u16();
                let hash_b = buffer.read_u16();
                let quad_a = self.state.chunk_map[hash_a as usize];
                let quad_b = self.state.chunk_map[hash_b as usize];
                (quad_a, quad_b)
            }

            // PLAIN_PLAIN_PLAIN_PLAIN_FLAGS => {
            //     let quad_a = buffer.read_u32();
            //     let quad_b = buffer.read_u32();
            //     let quad_c = buffer.read_u32();
            //     let quad_d = buffer.read_u32();
            //     let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_b = ((quad_b * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_c = ((quad_c * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_d = ((quad_d * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     self.state.chunk_map[hash_a as usize] = quad_a;
            //     self.state.chunk_map[hash_b as usize] = quad_b;
            //     self.state.chunk_map[hash_c as usize] = quad_c;
            //     self.state.chunk_map[hash_d as usize] = quad_d;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // PLAIN_PLAIN_PLAIN_MAP_FLAGS => {
            //     let quad_a = buffer.read_u32();
            //     let quad_b = buffer.read_u32();
            //     let quad_c = buffer.read_u32();
            //     let hash_d = buffer.read_u16();
            //     let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_b = ((quad_b * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_c = ((quad_c * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_d = self.state.chunk_map[hash_d as usize];
            //     self.state.chunk_map[hash_a as usize] = quad_a;
            //     self.state.chunk_map[hash_b as usize] = quad_b;
            //     self.state.chunk_map[hash_c as usize] = quad_c;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // PLAIN_PLAIN_MAP_PLAIN_FLAGS => {
            //     let quad_a = buffer.read_u32();
            //     let quad_b = buffer.read_u32();
            //     let hash_c = buffer.read_u16();
            //     let quad_d = buffer.read_u32();
            //     let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_b = ((quad_b * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_c = self.state.chunk_map[hash_c as usize];
            //     let hash_d = ((quad_d * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     self.state.chunk_map[hash_a as usize] = quad_a;
            //     self.state.chunk_map[hash_b as usize] = quad_b;
            //     self.state.chunk_map[hash_d as usize] = quad_d;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // PLAIN_PLAIN_MAP_MAP_FLAGS => {
            //     let quad_a = buffer.read_u32();
            //     let quad_b = buffer.read_u32();
            //     let hash_c = buffer.read_u16();
            //     let hash_d = buffer.read_u16();
            //     let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_b = ((quad_b * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_c = self.state.chunk_map[hash_c as usize];
            //     let quad_d = self.state.chunk_map[hash_d as usize];
            //     self.state.chunk_map[hash_a as usize] = quad_a;
            //     self.state.chunk_map[hash_b as usize] = quad_b;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // PLAIN_MAP_PLAIN_PLAIN_FLAGS => {
            //     let quad_a = buffer.read_u32();
            //     let hash_b = buffer.read_u16();
            //     let quad_c = buffer.read_u32();
            //     let quad_d = buffer.read_u32();
            //     let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_b = self.state.chunk_map[hash_b as usize];
            //     let hash_c = ((quad_c * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_d = ((quad_d * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     self.state.chunk_map[hash_a as usize] = quad_a;
            //     self.state.chunk_map[hash_c as usize] = quad_c;
            //     self.state.chunk_map[hash_d as usize] = quad_d;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // PLAIN_MAP_PLAIN_MAP_FLAGS => {
            //     let quad_a = buffer.read_u32();
            //     let hash_b = buffer.read_u16();
            //     let quad_c = buffer.read_u32();
            //     let hash_d = buffer.read_u16();
            //     let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_b = self.state.chunk_map[hash_b as usize];
            //     let hash_c = ((quad_c * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_d = self.state.chunk_map[hash_d as usize];
            //     self.state.chunk_map[hash_a as usize] = quad_a;
            //     self.state.chunk_map[hash_c as usize] = quad_c;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // PLAIN_MAP_MAP_PLAIN_FLAGS => {
            //     let quad_a = buffer.read_u32();
            //     let hash_b = buffer.read_u16();
            //     let hash_c = buffer.read_u16();
            //     let quad_d = buffer.read_u32();
            //     let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_b = self.state.chunk_map[hash_b as usize];
            //     let quad_c = self.state.chunk_map[hash_c as usize];
            //     let hash_d = ((quad_d * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     self.state.chunk_map[hash_a as usize] = quad_a;
            //     self.state.chunk_map[hash_d as usize] = quad_d;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // PLAIN_MAP_MAP_MAP_FLAGS => {
            //     let quad_a = buffer.read_u32();
            //     let hash_b = buffer.read_u16();
            //     let hash_c = buffer.read_u16();
            //     let hash_d = buffer.read_u16();
            //     let hash_a = ((quad_a * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_b = self.state.chunk_map[hash_b as usize];
            //     let quad_c = self.state.chunk_map[hash_c as usize];
            //     let quad_d = self.state.chunk_map[hash_d as usize];
            //     self.state.chunk_map[hash_a as usize] = quad_a;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // MAP_PLAIN_PLAIN_PLAIN_FLAGS => {
            //     let hash_a = buffer.read_u16();
            //     let quad_b = buffer.read_u32();
            //     let quad_c = buffer.read_u32();
            //     let quad_d = buffer.read_u32();
            //     let quad_a = self.state.chunk_map[hash_a as usize];
            //     let hash_b = ((quad_b * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_c = ((quad_c * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_d = ((quad_d * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     self.state.chunk_map[hash_b as usize] = quad_b;
            //     self.state.chunk_map[hash_c as usize] = quad_c;
            //     self.state.chunk_map[hash_d as usize] = quad_d;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // MAP_PLAIN_PLAIN_MAP_FLAGS => {
            //     let hash_a = buffer.read_u16();
            //     let quad_b = buffer.read_u32();
            //     let quad_c = buffer.read_u32();
            //     let hash_d = buffer.read_u16();
            //     let quad_a = self.state.chunk_map[hash_a as usize];
            //     let hash_b = ((quad_b * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_c = ((quad_c * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_d = self.state.chunk_map[hash_d as usize];
            //     self.state.chunk_map[hash_b as usize] = quad_b;
            //     self.state.chunk_map[hash_c as usize] = quad_c;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // MAP_PLAIN_MAP_PLAIN_FLAGS => {
            //     let hash_a = buffer.read_u16();
            //     let quad_b = buffer.read_u32();
            //     let hash_c = buffer.read_u16();
            //     let quad_d = buffer.read_u32();
            //     let quad_a = self.state.chunk_map[hash_a as usize];
            //     let hash_b = ((quad_b * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_c = self.state.chunk_map[hash_c as usize];
            //     let hash_d = ((quad_d * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     self.state.chunk_map[hash_b as usize] = quad_b;
            //     self.state.chunk_map[hash_d as usize] = quad_d;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // MAP_PLAIN_MAP_MAP_FLAGS => {
            //     let hash_a = buffer.read_u16();
            //     let quad_b = buffer.read_u32();
            //     let hash_c = buffer.read_u16();
            //     let hash_d = buffer.read_u16();
            //     let quad_a = self.state.chunk_map[hash_a as usize];
            //     let hash_b = ((quad_b * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_c = self.state.chunk_map[hash_c as usize];
            //     let quad_d = self.state.chunk_map[hash_d as usize];
            //     self.state.chunk_map[hash_b as usize] = quad_b;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // MAP_MAP_PLAIN_PLAIN_FLAGS => {
            //     let hash_a = buffer.read_u16();
            //     let hash_b = buffer.read_u16();
            //     let quad_c = buffer.read_u32();
            //     let quad_d = buffer.read_u32();
            //     let quad_a = self.state.chunk_map[hash_a as usize];
            //     let quad_b = self.state.chunk_map[hash_b as usize];
            //     let hash_c = ((quad_c * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let hash_d = ((quad_d * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     self.state.chunk_map[hash_c as usize] = quad_c;
            //     self.state.chunk_map[hash_d as usize] = quad_d;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // MAP_MAP_PLAIN_MAP_FLAGS => {
            //     let hash_a = buffer.read_u16();
            //     let hash_b = buffer.read_u16();
            //     let quad_c = buffer.read_u32();
            //     let hash_d = buffer.read_u16();
            //     let quad_a = self.state.chunk_map[hash_a as usize];
            //     let quad_b = self.state.chunk_map[hash_b as usize];
            //     let hash_c = ((quad_c * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     let quad_d = self.state.chunk_map[hash_d as usize];
            //     self.state.chunk_map[hash_c as usize] = quad_c;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // MAP_MAP_MAP_PLAIN_FLAGS => {
            //     let hash_a = buffer.read_u16();
            //     let hash_b = buffer.read_u16();
            //     let hash_c = buffer.read_u16();
            //     let quad_d = buffer.read_u32();
            //     let quad_a = self.state.chunk_map[hash_a as usize];
            //     let quad_b = self.state.chunk_map[hash_b as usize];
            //     let quad_c = self.state.chunk_map[hash_c as usize];
            //     let hash_d = ((quad_d * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
            //     self.state.chunk_map[hash_d as usize] = quad_d;
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
            // _ => {
            //     let hash_a = buffer.read_u16();
            //     let hash_b = buffer.read_u16();
            //     let hash_c = buffer.read_u16();
            //     let hash_d = buffer.read_u16();
            //     let quad_a = self.state.chunk_map[hash_a as usize];
            //     let quad_b = self.state.chunk_map[hash_b as usize];
            //     let quad_c = self.state.chunk_map[hash_c as usize];
            //     let quad_d = self.state.chunk_map[hash_d as usize];
            //     (quad_a, quad_b, quad_c, quad_d)
            // }
        }
    }
}

impl Codec for Chameleon {
    fn block_size(&self) -> usize { 256 }
}
