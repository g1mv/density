use crate::utils::file_path;
use divan;

mod utils;

fn main() {
    file_path(true);
    divan::main();
}

#[divan::bench_group(sample_count = 25)]
mod chameleon {
    use crate::utils::file_path;
    use density::algorithms::chameleon::chameleon::Chameleon;
    use divan::counter::BytesCount;
    use divan::Bencher;
    use std::fs::read;

    #[divan::bench(name = "compress/raw")]
    fn encode_raw(bencher: Bencher) {
        let file_mem = read(file_path(false)).unwrap();
        let mut encoded_mem = vec![0_u8; file_mem.len() << 1];

        print!("\r\t\t\t\x1b[1m\x1b[34m({:.3}x)\x1b[0m   ", file_mem.len() as f64 / Chameleon::encode(&file_mem, &mut encoded_mem).unwrap() as f64);

        bencher
            .counter(BytesCount::of_slice(&file_mem))
            .bench_local(|| { Chameleon::encode(&file_mem, &mut encoded_mem) });
    }

    #[divan::bench(name = "decompress/raw             ")]
    fn decode_raw(bencher: Bencher) {
        let file_mem = read(file_path(false)).unwrap();
        let mut encoded_mem = vec![0_u8; file_mem.len() << 1];
        let encoded_size = Chameleon::encode(&file_mem, &mut encoded_mem).unwrap();
        let mut decoded_mem = vec![0_u8; file_mem.len() << 1];

        let decoded_size = Chameleon::decode(&encoded_mem[0..encoded_size], &mut decoded_mem).unwrap();
        assert_eq!(file_mem.len(), decoded_size);
        for i in 0..decoded_size {
            assert_eq!(file_mem[i], decoded_mem[i]);
        }

        bencher
            .counter(BytesCount::of_slice(&file_mem))
            .bench_local(|| { Chameleon::decode(&encoded_mem[0..encoded_size], &mut decoded_mem) });
    }
}

#[divan::bench_group(sample_count = 25)]
mod cheetah {
    use crate::utils::file_path;
    use density::algorithms::cheetah::cheetah::Cheetah;
    use divan::counter::BytesCount;
    use divan::Bencher;
    use std::fs::read;

    #[divan::bench(name = "compress/raw")]
    fn encode_raw(bencher: Bencher) {
        let file_mem = read(file_path(false)).unwrap();
        let mut encoded_mem = vec![0_u8; file_mem.len() << 1];

        print!("\r\t\t\t\x1b[1m\x1b[34m({:.3}x)\x1b[0m   ", file_mem.len() as f64 / Cheetah::encode(&file_mem, &mut encoded_mem).unwrap() as f64);

        bencher
            .counter(BytesCount::of_slice(&file_mem))
            .bench_local(|| { Cheetah::encode(&file_mem, &mut encoded_mem) });
    }

    #[divan::bench(name = "decompress/raw")]
    fn decode_raw(bencher: Bencher) {
        let file_mem = read(file_path(false)).unwrap();
        let mut encoded_mem = vec![0_u8; file_mem.len() << 1];
        let encoded_size = Cheetah::encode(&file_mem, &mut encoded_mem).unwrap();
        let mut decoded_mem = vec![0_u8; file_mem.len() << 1];

        let decoded_size = Cheetah::decode(&encoded_mem[0..encoded_size], &mut decoded_mem).unwrap();
        assert_eq!(file_mem.len(), decoded_size);
        for i in 0..decoded_size {
            assert_eq!(file_mem[i], decoded_mem[i]);
        }

        bencher
            .counter(BytesCount::of_slice(&file_mem))
            .bench_local(|| { Cheetah::decode(&encoded_mem[0..encoded_size], &mut decoded_mem) });
    }
}