use std::fmt::{Display, Formatter};

#[derive(Debug)]
pub struct EncodeError {}

impl Display for EncodeError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Encode error")
    }
}

impl std::error::Error for EncodeError {}