use std::env;


pub fn file_path() -> String {
    env::var("FILE").unwrap().to_owned()
}