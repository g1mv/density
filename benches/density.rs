use std::fs::read_to_string;
use std::io;

use divan::Bencher;

use density::algorithms::chameleon::Chameleon;
use density::algorithms::chameleon_writer::ChameleonWriter;
use density::codec::Codec;

fn main() {
    divan::main();
}

fn compress_raw_safe(bencher: Bencher, path: &str) {
    let in_memory_json = read_to_string(path).unwrap();
    let input = in_memory_json.as_bytes();
    let mut output = vec![0_u8; in_memory_json.len() << 1];

    let mut chameleon = Chameleon::new();
    match chameleon.encode_safe(input, &mut output) {
        Ok(bytes) => { println!("{} bytes", bytes) }
        Err(_) => { assert!(false); }
    }

    bencher.bench_local(move || {
        let mut chameleon = Chameleon::new();
        chameleon.encode_safe(input, &mut output)
    });

    // assert!(std::io::copy(&mut input, &mut ChameleonWriter::new(&mut output)).is_ok());
    // bencher.with_inputs(|| {
    //     let binding = vec![0; in_memory_json.len() << 1];
    //     (in_memory_json.to_owned(), binding)
    // }).bench_local_values(move |(input, mut output)| {
    //     io::copy(&mut input.as_bytes(), &mut ChameleonWriter::new(&mut output))
    // });
}

fn compress_raw_unsafe(bencher: Bencher, path: &str) {
    let in_memory_json = read_to_string(path).unwrap();
    let input = in_memory_json.as_bytes();
    let mut output = vec![0_u8; in_memory_json.len() << 1];

    let mut chameleon = Chameleon::new();
    match unsafe { chameleon.encode_unsafe(input, &mut output) } {
        Ok(bytes) => { println!("{} bytes", bytes) }
        Err(_) => { assert!(false); }
    }

    bencher.bench_local(move || {
        let mut chameleon = Chameleon::new();
        unsafe { chameleon.encode_unsafe(input, &mut output) }
    });
}

fn compress_stream_safe(bencher: Bencher, path: &str) {
    let in_memory_json = read_to_string(path).unwrap();
    let mut input = in_memory_json.as_bytes();
    let mut output = Vec::with_capacity(in_memory_json.len() << 1); // vec![0_u8; in_memory_json.len() << 1]; // Vec::new();//

    assert!(std::io::copy(&mut input, &mut ChameleonWriter::new(&mut output)).is_ok());
    println!("{} bytes", output.len());
    bencher.with_inputs(|| {
        let binding = Vec::with_capacity(in_memory_json.len() << 1);
        (in_memory_json.to_owned(), binding)
    }).bench_local_values(move |(input, mut output)| {
        io::copy(&mut input.as_bytes(), &mut ChameleonWriter::new(&mut output))
    });
}

#[divan::bench(args = ["benches/data/enwik8"])]
fn encode_raw_safe(bencher: Bencher, path: &str) {
    compress_raw_safe(bencher, path);
}

#[divan::bench(args = ["benches/data/enwik8"])]
fn encode_raw_unsafe(bencher: Bencher, path: &str) {
    compress_raw_unsafe(bencher, path);
}

#[divan::bench(args = ["benches/data/enwik8"])]
fn encode_stream_safe(bencher: Bencher, path: &str) {
    compress_stream_safe(bencher, path);
}