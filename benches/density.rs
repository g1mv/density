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
    use density_rs::algorithms::chameleon::chameleon::Chameleon;
    use density_rs::codec::codec::Codec;
    use divan::counter::BytesCount;
    use divan::Bencher;
    use std::fs::read;

    #[divan::bench(name = "compress/raw")]
    fn encode_raw(bencher: Bencher) {
        let file_mem = read(file_path(false)).unwrap();

        let mut chameleon = Chameleon::new();
        let mut encoded_mem = vec![0_u8; chameleon.safe_encode_buffer_size(file_mem.len())];

        print!("\r\t\t\t(\x1b[1m\x1b[34m{:.3}x\x1b[0m)   ", file_mem.len() as f64 / chameleon.encode(&file_mem, &mut encoded_mem).unwrap() as f64);

        bencher
            .counter(BytesCount::of_slice(&file_mem))
            .bench_local(|| { Chameleon::encode(&file_mem, &mut encoded_mem) });
    }

    #[divan::bench(name = "decompress/raw             ")]
    fn decode_raw(bencher: Bencher) {
        let file_mem = read(file_path(false)).unwrap();

        let mut chameleon = Chameleon::new();
        let mut encoded_mem = vec![0_u8; chameleon.safe_encode_buffer_size(file_mem.len())];
        let encoded_size = chameleon.encode(&file_mem, &mut encoded_mem).unwrap();

        let mut decoded_mem = vec![0_u8; file_mem.len()];
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
    use density_rs::algorithms::cheetah::cheetah::Cheetah;
    use density_rs::codec::codec::Codec;
    use divan::counter::BytesCount;
    use divan::Bencher;
    use std::fs::read;

    #[divan::bench(name = "compress/raw")]
    fn encode_raw(bencher: Bencher) {
        let file_mem = read(file_path(false)).unwrap();

        let mut cheetah = Cheetah::new();
        let mut encoded_mem = vec![0_u8; cheetah.safe_encode_buffer_size(file_mem.len())];

        print!("\r\t\t\t(\x1b[1m\x1b[34m{:.3}x\x1b[0m)   ", file_mem.len() as f64 / cheetah.encode(&file_mem, &mut encoded_mem).unwrap() as f64);

        bencher
            .counter(BytesCount::of_slice(&file_mem))
            .bench_local(|| { Cheetah::encode(&file_mem, &mut encoded_mem) });
    }

    #[divan::bench(name = "decompress/raw")]
    fn decode_raw(bencher: Bencher) {
        let file_mem = read(file_path(false)).unwrap();

        let mut cheetah = Cheetah::new();
        let mut encoded_mem = vec![0_u8; cheetah.safe_encode_buffer_size(file_mem.len())];
        let encoded_size = cheetah.encode(&file_mem, &mut encoded_mem).unwrap();

        let mut decoded_mem = vec![0_u8; file_mem.len()];
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

#[divan::bench_group(sample_count = 25)]
mod lion {
    use crate::utils::file_path;
    use density_rs::algorithms::lion::lion::Lion;
    use density_rs::codec::codec::Codec;
    use divan::counter::BytesCount;
    use divan::Bencher;
    use std::fs::read;

    #[divan::bench(name = "compress/raw")]
    fn encode_raw(bencher: Bencher) {
        let file_mem = read(file_path(false)).unwrap();

        let mut lion = Lion::new();
        let mut encoded_mem = vec![0_u8; lion.safe_encode_buffer_size(file_mem.len())];

        print!("\r\t\t\t(\x1b[1m\x1b[34m{:.3}x\x1b[0m)   ", file_mem.len() as f64 / lion.encode(&file_mem, &mut encoded_mem).unwrap() as f64);

        bencher
            .counter(BytesCount::of_slice(&file_mem))
            .bench_local(|| { Lion::encode(&file_mem, &mut encoded_mem) });
    }

    #[divan::bench(name = "decompress/raw")]
    fn decode_raw(bencher: Bencher) {
        let file_mem = read(file_path(false)).unwrap();

        let mut lion = Lion::new();
        let mut encoded_mem = vec![0_u8; lion.safe_encode_buffer_size(file_mem.len())];
        let encoded_size = lion.encode(&file_mem, &mut encoded_mem).unwrap();

        let mut decoded_mem = vec![0_u8; file_mem.len()];
        let decoded_size = Lion::decode(&encoded_mem[0..encoded_size], &mut decoded_mem).unwrap();
        assert_eq!(file_mem.len(), decoded_size);
        for i in 0..decoded_size {
            assert_eq!(file_mem[i], decoded_mem[i]);
        }

        bencher
            .counter(BytesCount::of_slice(&file_mem))
            .bench_local(|| { Lion::decode(&encoded_mem[0..encoded_size], &mut decoded_mem) });
    }
}