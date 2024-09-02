use std::env;
use std::fs::metadata;

pub fn file_path(display: bool) -> String {
    let file_path = env::var("FILE").unwrap();
    if display {
        let size = metadata(&file_path).unwrap().len();
        println!("Using file \x1b[1m{}\x1b[0m (\x1b[37m{} bytes\x1b[0m)", &file_path, size);
    }

    file_path
}