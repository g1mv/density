use crate::common::prepare_file;

mod common;

fn main() {
    prepare_file();
    divan::main();
}

#[divan::bench_group(sample_count = 25)]
mod chameleon {
    use crate::common::{DATA_DIRECTORY, TEST_FILE};
    use density::algorithms::chameleon::Chameleon;
    use ::density::algorithms::chameleon_writer::ChameleonWriter;
    use divan::counter::BytesCount;
    use divan::Bencher;
    use std::fs::read;
    use std::io;

    #[divan::bench(name = "compress/raw")]
    fn encode_raw(bencher: Bencher) {
        let in_mem = read(&format!("{}{}", DATA_DIRECTORY, TEST_FILE)).unwrap();
        let mut out_mem = vec![0_u8; in_mem.len() << 1];

        print!("\r\t\t\t({:.3}x)   ", in_mem.len() as f64 / Chameleon::encode(&in_mem, &mut out_mem).unwrap() as f64);

        bencher
            .counter(BytesCount::of_slice(&in_mem))
            .bench_local(|| { Chameleon::encode(&in_mem, &mut out_mem) });
    }

    #[divan::bench(name = "compress/stream            ")]
    fn encode_stream(bencher: Bencher) {
        let in_mem = read(&format!("{}{}", DATA_DIRECTORY, TEST_FILE)).unwrap();
        let capacity = in_mem.len() << 1;
        let mut out_mem = Vec::with_capacity(capacity);

        let mut writer = ChameleonWriter::new(&mut out_mem);
        io::copy(&mut in_mem.as_slice(), &mut writer).unwrap();

        print!("\r\t\t\t({:.3}x)   ", in_mem.len() as f64 / writer.writer.len() as f64);

        bencher
            .counter(BytesCount::of_slice(&in_mem))
            .with_inputs(|| { (in_mem.as_slice(), Vec::with_capacity(capacity)) })
            .bench_local_values(|(mut input, mut output)| {
                io::copy(&mut input, &mut ChameleonWriter::new(&mut output))
            });
    }
}

#[divan::bench_group(sample_count = 25)]
mod cheetah {
    use crate::common::{DATA_DIRECTORY, TEST_FILE};
    use density::algorithms::cheetah::Cheetah;
    use divan::counter::BytesCount;
    use divan::Bencher;
    use std::fs::read;

    #[divan::bench(name = "compress/raw")]
    fn encode_raw(bencher: Bencher) {
        let in_mem = read(&format!("{}{}", DATA_DIRECTORY, TEST_FILE)).unwrap();
        let mut out_mem = vec![0_u8; in_mem.len() << 1];

        print!("\r\t\t\t({:.3}x)   ", in_mem.len() as f64 / Cheetah::encode(&in_mem, &mut out_mem).unwrap() as f64);

        bencher
            .counter(BytesCount::of_slice(&in_mem))
            .bench_local(|| { Cheetah::encode(&in_mem, &mut out_mem) });
    }
}

//
// fn encode_raw2(c: &mut Criterion) {
//     for path in TEST_FILES {
//         let in_mem = read(&format!("{}{}", DATA_DIRECTORY, path)).unwrap();
//         let mut out_mem = vec![0_u8; in_mem.len() << 1];
//
//         let out_size = Chameleon::encode(&in_mem, &mut out_mem).unwrap();
//
//         let mut group = c.benchmark_group("density/chameleon");
//         group.sampling_mode(SamplingMode::Flat);
//         group.sample_size(10);
//         group.throughput(Throughput::Bytes(in_mem.len() as u64));
//         group.bench_function(&format!("encode_raw {} ({} bytes => {} bytes)", path, in_mem.len(), out_size), |b| b.iter(|| {
//             Chameleon::encode(&in_mem, &mut out_mem)
//         }));
//         group.finish();
//     }
// }
//
// fn encode_stream2(c: &mut Criterion) {
//     for path in TEST_FILES {
//         let in_mem = read(&format!("{}{}", DATA_DIRECTORY, path)).unwrap();
//         let mut out_mem = Vec::with_capacity(in_mem.len() << 1);
//
//         io::copy(&mut in_mem.as_slice(), &mut ChameleonWriter::new(&mut out_mem)).unwrap();
//
//         let mut group = c.benchmark_group("density/chameleon");
//         group.sampling_mode(SamplingMode::Flat);
//         group.sample_size(10);
//         group.throughput(Throughput::Bytes(in_mem.len() as u64));
//         group.bench_function(&format!("encode_stream {} ({} bytes => {} bytes)", path, in_mem.len(), out_mem.len()), |b| b.iter(|| {
//             io::copy(&mut in_mem.as_slice(), &mut ChameleonWriter::new(&mut Vec::with_capacity(in_mem.len() << 1)))
//         }));
//         group.finish();
//     }
// }

// criterion_group!(benches, encode_raw2, encode_stream2);
// criterion_main!(benches);