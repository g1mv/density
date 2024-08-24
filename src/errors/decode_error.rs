use std::fmt::{Display, Formatter};

#[derive(Debug)]
pub struct DecodeError {}

impl Display for DecodeError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        todo!()
    }
}

impl std::error::Error for DecodeError {}