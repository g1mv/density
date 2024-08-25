pub struct ReadSignature {
    pub(crate) value: u64,
    pub(crate) shift: u8,
}

impl ReadSignature {
    pub fn new(value: u64) -> Self {
        ReadSignature { value, shift: 0 }
    }

    #[inline(always)]
    pub fn read_bits(&mut self, mask: u64, n: u8) -> u64 {
        let value = (self.value >> self.shift) & mask;
        self.shift += n;
        value
    }
}