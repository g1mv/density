pub mod codec;
pub mod algorithms;
pub mod buffer;
pub mod errors;
pub mod io;

pub(crate) const BYTE_SIZE_U32: usize = size_of::<u32>();
pub(crate) const BIT_SIZE_U32: usize = BYTE_SIZE_U32 << 3;
pub(crate) const BYTE_SIZE_U64: usize = size_of::<u64>();
pub(crate) const BYTE_SIZE_U128: usize = size_of::<u128>();

#[cfg(test)]
mod tests {
    use crate::algorithms::chameleon::chameleon::Chameleon;

    const TEST_DATA: &str = "testtesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttestt";

    #[test]
    fn chameleon() {
        assert_eq!(TEST_DATA.len(), 125);
        let mut out_mem = vec![0; TEST_DATA.len()];
        match Chameleon::encode(TEST_DATA.as_bytes(), &mut out_mem) {
            Ok(size) => {
                assert_eq!(&out_mem[0..size], [0xfe, 0xff, 0xff, 0xff, 0, 0, 0, 0, 116, 101, 115, 116, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 116, 0, 0, 0, 0, 0, 0, 0, 0]);
            }
            Err(_) => { assert!(false); }
        }
    }
}

