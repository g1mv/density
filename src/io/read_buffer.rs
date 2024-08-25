pub struct ReadBuffer<'a> {
    pub buffer: &'a [u8],
    pub index: usize,
}

impl<'a> ReadBuffer<'a> {
    pub fn new(buffer: &'a [u8]) -> Self {
        ReadBuffer {
            buffer,
            index: 0,
        }
    }

    #[inline(always)]
    pub fn remaining(&self) -> usize {
        self.buffer.len() - self.index
    }

    #[inline(always)]
    pub fn read(&mut self, n: usize) -> &[u8] {
        let start_index = self.index;
        let end_index = self.index + n;
        self.index = end_index;
        &self.buffer[start_index..end_index]
    }

    #[inline(always)]
    pub fn read_u64(&mut self) -> u64 {
        let start_index = self.index;
        let end_index = self.index + size_of::<u64>();
        self.index = end_index;
        u64::from_le_bytes(<&[u8] as TryInto<[u8; size_of::<u64>()]>>::try_into(&self.buffer[start_index..end_index]).unwrap())
    }

    #[inline(always)]
    pub fn read_u32(&mut self) -> u32 {
        let start_index = self.index;
        let end_index = self.index + size_of::<u32>();
        self.index = end_index;
        u32::from_le_bytes(<&[u8] as TryInto<[u8; size_of::<u32>()]>>::try_into(&self.buffer[start_index..end_index]).unwrap())
    }

    #[inline(always)]
    pub fn read_u16(&mut self) -> u16 {
        let start_index = self.index;
        let end_index = self.index + size_of::<u16>();
        self.index = end_index;
        u16::from_le_bytes(<&[u8] as TryInto<[u8; size_of::<u16>()]>>::try_into(&self.buffer[start_index..end_index]).unwrap())
    }
}