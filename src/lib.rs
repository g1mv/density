pub mod codec;
pub mod algorithms;
pub mod buffer;
pub mod errors;
pub mod io;

pub(crate) const BYTE_SIZE_U32: usize = size_of::<u32>();
pub(crate) const BIT_SIZE_U32: usize = BYTE_SIZE_U32 << 3;
pub(crate) const BYTE_SIZE_U64: usize = size_of::<u64>();
pub(crate) const BYTE_SIZE_U128: usize = size_of::<u128>();
pub(crate) const BYTE_SIZE_SIGNATURE: usize = BYTE_SIZE_U64;

#[cfg(test)]
mod tests {
    use crate::algorithms::chameleon::chameleon::Chameleon;
    use crate::algorithms::cheetah::cheetah::Cheetah;

    const TEST_DATA: &str = "testtesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttestt";

    #[test]
    fn chameleon() {
        let mut out_mem = vec![0; TEST_DATA.len()];

        // Encoding
        match Chameleon::encode(TEST_DATA.as_bytes(), &mut out_mem) {
            Ok(size) => {
                assert_eq!(&out_mem[0..size], [0xfe, 0xff, 0xff, 0x7f, 0, 0, 0, 0, 116, 101, 115, 116, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 112, 251, 116]);

                // Decoding
                let mut dec_mem = vec![0; TEST_DATA.len()];
                match Chameleon::decode(&out_mem[0..size], &mut dec_mem) {
                    Ok(size) => {
                        assert_eq!(&dec_mem[0..size], TEST_DATA.as_bytes());
                    }
                    Err(_) => { assert!(false); }
                }
            }
            Err(_) => { assert!(false); }
        }
    }
    #[test]
    fn cheetah() {
        let mut out_mem = vec![0; TEST_DATA.len()];

        // Encoding
        match Cheetah::encode(TEST_DATA.as_bytes(), &mut out_mem) {
            Ok(size) => {
                assert_eq!(&out_mem[0..size], [244, 255, 255, 255, 255, 255, 255, 63, 116, 101, 115, 116, 112, 251, 116]);

                // Decoding
                let mut dec_mem = vec![0; TEST_DATA.len()];
                match Cheetah::decode(&out_mem[0..size], &mut dec_mem) {
                    Ok(size) => {
                        assert_eq!(&dec_mem[0..size], TEST_DATA.as_bytes());
                    }
                    Err(_) => { assert!(false); }
                }
            }
            Err(_) => { assert!(false); }
        }
    }
}

