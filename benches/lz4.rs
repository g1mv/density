use crate::common::prepare_file;

mod common;

fn main() {
    prepare_file();
    divan::main();
}

#[divan::bench_group(sample_count = 25)]
mod lz4 {
    use crate::common::{DATA_DIRECTORY, TEST_FILE};
    use divan::counter::BytesCount;
    use divan::Bencher;
    use lz4_flex::compress_into;
    use std::fs::read;

    #[divan::bench(name = "compress/raw               ")]
    fn encode_raw(bencher: Bencher) {
        let in_mem = read(&format!("{}{}", DATA_DIRECTORY, TEST_FILE)).unwrap();
        let mut out_mem = vec![0_u8; in_mem.len() << 1];

        print!("\r\t\t\t({:.3}x)   ", in_mem.len() as f64 / compress_into(&in_mem, &mut out_mem).unwrap() as f64);

        bencher
            .counter(BytesCount::of_slice(&in_mem))
            .bench_local(|| { compress_into(&in_mem, &mut out_mem) });
    }
}

//
// fn encode_raw(c: &mut Criterion) {
//     for path in TEST_FILES {
//         let in_mem = read(&format!("{}{}", DATA_DIRECTORY, path)).unwrap();
//         let mut out_mem = vec![0_u8; in_mem.len() << 1];
//
//         let out_size = compress_into(&in_mem, &mut out_mem).unwrap();
//
//         let mut group = c.benchmark_group("lz4");
//         group.sampling_mode(SamplingMode::Flat);
//         group.sample_size(10);
//         group.throughput(Throughput::Bytes(in_mem.len() as u64));
//         group.bench_function(&format!("encode_raw {} ({} bytes => {} bytes)", path, in_mem.len(), out_size), |b| b.iter(|| {
//             compress_into(&in_mem, &mut out_mem)
//         }));
//         group.finish();
//     }
// }
//
// criterion_group!(benches, encode_raw);
// criterion_main!(benches);