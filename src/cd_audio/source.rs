#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum CdSource {
    #[default]
    Auto,
    Files,
    Mci,
}

impl CdSource {
    pub fn from_str(s: &str) -> Self {
        match s.to_ascii_lowercase().as_str() {
            "files" => Self::Files,
            "mci" => Self::Mci,
            _ => Self::Auto,
        }
    }
}
