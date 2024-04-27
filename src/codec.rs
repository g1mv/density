use crate::error::Error;

pub trait Codec {
    unsafe fn encode(&mut self, input: &[u8], output: &mut [u8]) -> Result<(), Error>;
    unsafe fn decode(&mut self, input: &[u8], output: &mut [u8]) -> Result<(), Error>;
}