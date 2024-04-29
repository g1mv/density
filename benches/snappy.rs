use std::fs::read_to_string;
use std::io;

use divan::Bencher;

fn main() {
    divan::main();
}

fn compress_file(bencher: Bencher, path: &str) {
    let in_memory_json = read_to_string(path).unwrap();

    let copy = in_memory_json.to_owned();
    let mut binding = vec![0; in_memory_json.len() << 1];
    assert!({ io::copy(&mut copy.as_bytes(), &mut snap::write::FrameEncoder::new(&mut binding)).is_ok() });

    bencher.with_inputs(|| {
        let binding = vec![0; in_memory_json.len() << 1];
        (in_memory_json.to_owned(), binding)
    }).bench_local_values(move |(input, mut output)| {
        io::copy(&mut input.as_bytes(), &mut snap::write::FrameEncoder::new(&mut output))
    });
}

#[divan::bench(args = ["benches/data/enwik8"])]
fn compress(bencher: Bencher, path: &str) {
    compress_file(bencher, path);
}