use crate::error::Error;

pub trait Codec {
    unsafe fn encode_unsafe(&mut self, input: &[u8], output: &mut [u8]) -> Result<usize, Error>;
    unsafe fn decode(&mut self, input: &[u8], output: &mut [u8]) -> Result<(), Error>;
}