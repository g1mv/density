use crate::codec::Codec;
use crate::encode_buffer::EncodeBuffer;
use crate::errors::encode_error::EncodeError;
use crate::quad_encoder::QuadEncoder;
use crate::signature::Signature;
use std::mem::size_of;

pub(crate) const CHAMELEON_HASH_BITS: usize = 16;
pub(crate) const CHAMELEON_HASH_MULTIPLIER: u32 = 0x9D6EF916;
pub(crate) const BYTE_SIZE_U16: usize = size_of::<u16>();
pub(crate) const BYTE_SIZE_U32: usize = size_of::<u32>();
pub(crate) const BIT_SIZE_U32: usize = BYTE_SIZE_U32 << 3;
pub(crate) const BYTE_SIZE_U64: usize = size_of::<u64>();
pub(crate) const BYTE_SIZE_U128: usize = size_of::<u128>();

pub struct State {
    pub(crate) chunk_map: [u32; 1 << CHAMELEON_HASH_BITS],
}

pub struct Chameleon {
    pub state: State,
}

impl Chameleon {
    pub fn encode(input: &[u8], output: &mut [u8]) -> Result<usize, EncodeError> {
        let mut chameleon = Chameleon::new();
        chameleon.encode(input, output)
    }

    pub fn new() -> Self {
        Chameleon {
            state: State { chunk_map: [0; 1 << CHAMELEON_HASH_BITS] },
        }
    }
}

impl QuadEncoder for Chameleon {
    #[inline(always)]
    fn encode_quad(&mut self, quad: u32, buffer: &mut EncodeBuffer, signature: &mut Signature) {
        let hash_u16 = ((quad * CHAMELEON_HASH_MULTIPLIER) >> (BIT_SIZE_U32 - CHAMELEON_HASH_BITS)) as u16;
        let dictionary_value = &mut self.state.chunk_map[hash_u16 as usize];
        if *dictionary_value != quad {
            buffer.push(&quad.to_le_bytes());
            if signature.push_bits(0, 1) {
                buffer.ink(signature);
            }
            *dictionary_value = quad;
        } else {
            buffer.push(&hash_u16.to_le_bytes());
            if signature.push_bits(1, 1) {
                buffer.ink(signature);
            }
        }
    }
}

impl Codec for Chameleon {}
