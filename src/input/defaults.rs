use super::profile::Profile;

/// The original game's keyboard and mouse controls
const CLASSIC: &str = include_str!("defaults/classic.toml");

fn load(name: &str, text: &str) -> Profile {
    match Profile::from_toml(text) {
        Ok((profile, _)) => profile,
        Err(e) => {
            tracing::error!("The {name} layout can't be read: {e}");
            Profile::default()
        }
    }
}

/// The original game's keyboard and mouse controls
pub fn classic() -> Profile {
    load("classic", CLASSIC)
}
