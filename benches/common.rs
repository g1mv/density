use std::fs::read;
use std::io::Read;
use std::path::Path;
use std::{env, fs};
use xz::read::XzDecoder;

pub const DATA_DIRECTORY: &str = "benches/data/";
pub const DEFAULT_TEST_FILE: &str = "enwik8";

pub fn test_file_path() -> String {
    let args = env::args().collect::<Vec<String>>();
    if args.len() > 2 {
        args.get(2).unwrap().to_owned()
    } else {
        format!("{}{}", DATA_DIRECTORY, DEFAULT_TEST_FILE)
    }
}

pub fn prepare_default_test_file() {
    // println!("{:?}", test_file_path());
    if &test_file_path() == DEFAULT_TEST_FILE {
        if !Path::new(&test_file_path()).exists() {
            let in_mem = read(&format!("{}.xz", test_file_path())).unwrap();
            println!("Decompressing {}.xz ({} bytes)...", test_file_path(), in_mem.len());
            let mut decoder = XzDecoder::new(in_mem.as_slice());
            let mut out_mem = Vec::with_capacity(in_mem.len() << 3);
            decoder.read_to_end(&mut out_mem).unwrap();
            println!("Decompressed to {} ({} bytes)", test_file_path(), out_mem.len());
            fs::write(&format!("{}", test_file_path()), out_mem).unwrap();
        }
    }

    let in_mem = read(test_file_path()).unwrap();
    println!("Using file {} ({} bytes)", test_file_path(), in_mem.len());
}