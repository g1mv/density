use crate::common::prepare_default_test_file;

mod common;

fn main() {
    prepare_default_test_file();
    divan::main();
}

#[divan::bench_group(sample_count = 25)]
mod default {
    use crate::common::test_file_path;
    use divan::counter::BytesCount;
    use divan::Bencher;
    use lz4_flex::{compress_into, decompress_into};
    use std::fs::read;

    #[divan::bench(name = "compress/raw")]
    fn encode_raw(bencher: Bencher) {
        let in_mem = read(test_file_path()).unwrap();
        let mut out_mem = vec![0_u8; in_mem.len() << 1];

        print!("\r\t\t\t({:.3}x)   ", in_mem.len() as f64 / compress_into(&in_mem, &mut out_mem).unwrap() as f64);

        bencher
            .counter(BytesCount::of_slice(&in_mem))
            .bench_local(|| { compress_into(&in_mem, &mut out_mem) });
    }

    #[divan::bench(name = "decompress/raw             ")]
    fn decode_raw(bencher: Bencher) {
        let in_mem = read(test_file_path()).unwrap();
        let mut out_mem = vec![0_u8; in_mem.len() << 1];
        let size = compress_into(&in_mem, &mut out_mem).unwrap();
        let mut dec_mem = vec![0_u8; in_mem.len() << 1];

        print!("\r\t\t\t({:.3}x)   ", in_mem.len() as f64 / decompress_into(&out_mem[0..size], &mut dec_mem).unwrap() as f64);

        bencher
            .counter(BytesCount::of_slice(&in_mem))
            .bench_local(|| { decompress_into(&out_mem[0..size], &mut dec_mem) });
    }
    //
    // #[divan::bench(name = "roundtrip/raw")]
    // fn roundtrip_raw(bencher: Bencher) {
    //     let in_mem = read(&format!("{}{}", DATA_DIRECTORY, TEST_FILE)).unwrap();
    //     let mut out_mem = vec![0_u8; in_mem.len() << 1];
    //     let mut dec_mem = vec![0_u8; in_mem.len() << 1];
    //
    //     bencher
    //         .counter(BytesCount::new(in_mem.len() * 2))
    //         .bench_local(|| {
    //             let size = compress_into(&in_mem, &mut out_mem).unwrap();
    //             decompress_into(&out_mem[0..size], &mut dec_mem)
    //         });
    // }
}