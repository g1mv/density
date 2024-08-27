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
    use snap::read::FrameDecoder;
    use snap::write::FrameEncoder;
    use std::fs::read;
    use std::io;

    #[divan::bench(name = "compress/stream            ")]
    fn encode_stream(bencher: Bencher) {
        let in_mem = read(test_file_path()).unwrap();
        let capacity = in_mem.len() << 1;
        let mut out_mem = Vec::with_capacity(capacity);

        let mut writer = FrameEncoder::new(&mut out_mem);
        io::copy(&mut in_mem.as_slice(), &mut writer).unwrap();

        print!("\r\t\t\t({:.3}x)   ", in_mem.len() as f64 / writer.get_ref().len() as f64);

        bencher
            .counter(BytesCount::of_slice(&in_mem))
            .with_inputs(|| { (in_mem.as_slice(), Vec::with_capacity(capacity)) })
            .bench_local_values(|(mut input, mut output)| {
                io::copy(&mut input, &mut FrameEncoder::new(&mut output))
            });
    }

    #[divan::bench(name = "decompress/stream          ")]
    fn decode_raw(bencher: Bencher) {
        let in_mem = read(test_file_path()).unwrap();
        let capacity = in_mem.len() << 1;
        let mut out_mem = Vec::with_capacity(capacity);
        let mut dec_mem = Vec::with_capacity(capacity);

        {
            let mut encoder = FrameEncoder::new(&mut out_mem);
            io::copy(&mut in_mem.as_slice(), &mut encoder).unwrap();
        }

        {
            let mut decoder = FrameDecoder::new(out_mem.as_slice());
            io::copy(&mut decoder, &mut dec_mem).unwrap();
        }

        print!("\r\t\t\t({:.3}x)   ", in_mem.len() as f64 / dec_mem.len() as f64);

        bencher
            .counter(BytesCount::of_slice(&in_mem))
            .with_inputs(|| { (out_mem.as_slice(), Vec::with_capacity(capacity)) })
            .bench_local_values(|(mut input, mut output)| {
                io::copy(&mut FrameDecoder::new(&mut input), &mut output)
            });
    }
}