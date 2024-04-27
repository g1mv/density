use std::fs::read_to_string;

use divan::Bencher;
use lz4_flex::{compress_into, compress_prepend_size};
use density::codec::Codec;

fn main() {
    divan::main();
}

fn compress_file(bencher: Bencher, path: &str) {
    let in_memory_json = read_to_string(path).unwrap();
    let input = in_memory_json.as_bytes();
    let mut binding = vec![0; in_memory_json.len() << 1];
    let output = binding.as_mut_slice();

    assert!(unsafe {
        let result = compress_into(input, output);
        if let Ok(bytes) = result {
            // println!("{} bytes", bytes)
        }
        result.is_ok()
    });

    bencher.bench_local(move || unsafe {
        compress_into(input, output)
    });
}

#[divan::bench(args = ["benches/data/enwik8"])]
fn compress(bencher: Bencher, path: &str) {
    compress_file(bencher, path);
}