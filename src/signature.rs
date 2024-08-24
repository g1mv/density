pub struct Signature {
    pub(crate) pos: usize,
    pub(crate) value: u64,
    pub(crate) shift: u8,
}

impl Signature {
    pub fn new(pos: usize) -> Self {
        Signature { pos, value: 0, shift: 0 }
    }

    #[inline(always)]
    pub fn push_bits(&mut self, mask: u64, shift: u8) -> bool {
        self.value |= mask << self.shift;
        let future_shift = self.shift + shift;
        if future_shift > 0x3f {
            true
        } else {
            self.shift = future_shift;
            false
        }
    }

    #[inline(always)]
    pub fn init(&mut self, pos: usize) {
        self.pos = pos;
        self.value = 0;
        self.shift = 0;
    }
}