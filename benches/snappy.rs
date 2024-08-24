use crate::common::prepare_file;

mod common;

fn main() {
    prepare_file();
    divan::main();
}

#[divan::bench_group(sample_count = 25)]
mod snappy {
    use crate::common::{DATA_DIRECTORY, TEST_FILE};
    use divan::counter::BytesCount;
    use divan::Bencher;
    use snap::write::FrameEncoder;
    use std::fs::read;
    use std::io;

    #[divan::bench(name = "compress/stream            ")]
    fn encode_stream(bencher: Bencher) {
        let in_mem = read(&format!("{}{}", DATA_DIRECTORY, TEST_FILE)).unwrap();
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
}
//
// fn encode_stream(c: &mut Criterion) {
//     for path in TEST_FILES {
//         let in_mem = read(&format!("{}{}", DATA_DIRECTORY, path)).unwrap();
//         let mut out_mem = Vec::with_capacity(in_mem.len() << 1);
//
//         io::copy(&mut in_mem.as_slice(), &mut snap::write::FrameEncoder::new(&mut out_mem)).unwrap();
//
//         let mut group = c.benchmark_group("snappy");
//         group.sampling_mode(SamplingMode::Flat);
//         group.sample_size(10);
//         group.throughput(Throughput::Bytes(in_mem.len() as u64));
//         group.bench_function(&format!("encode_stream {} ({} bytes => {} bytes)", path, in_mem.len(), out_mem.len()), |b| b.iter(|| {
//             io::copy(&mut in_mem.as_slice(), &mut snap::write::FrameEncoder::new(&mut Vec::with_capacity(in_mem.len() << 1)))
//         }));
//         group.finish();
//     }
// }
//
// criterion_group!(benches, encode_stream);
// criterion_main!(benches);