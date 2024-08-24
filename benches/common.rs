use std::fs;
use std::fs::read;
use std::io::Read;
use std::path::Path;
use xz::read::XzDecoder;

pub const DATA_DIRECTORY: &str = "benches/data/";
pub const TEST_FILE: &str = "enwik8";

pub fn prepare_file() {
    let silesia_tar_path = &format!("{}{}", DATA_DIRECTORY, TEST_FILE);

    if !Path::new(silesia_tar_path).exists() {
        let in_mem = read(&format!("{}.xz", silesia_tar_path)).unwrap();
        println!("Decompressing {}.xz ({} bytes)...", silesia_tar_path, in_mem.len());
        let mut decoder = XzDecoder::new(in_mem.as_slice());
        let mut out_mem = Vec::with_capacity(in_mem.len() << 3);
        decoder.read_to_end(&mut out_mem).unwrap();
        println!("Decompressed to {} ({} bytes)", silesia_tar_path, out_mem.len());
        fs::write(&format!("{}", silesia_tar_path), out_mem).unwrap();
    }

    let in_mem = read(silesia_tar_path).unwrap();
    println!("Using file {} ({} bytes)", silesia_tar_path, in_mem.len());
}