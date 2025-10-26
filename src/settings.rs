use std::sync::{LazyLock, Mutex};

use ini::Ini;

pub static SETTINGS: LazyLock<GlobalSettings> = LazyLock::new(|| GlobalSettings::load());

pub struct GlobalSettings {
    settings: Mutex<Ini>,
}

impl GlobalSettings {
    const SETTINGS_FILE_NAME: &str = "remech2.ini";

    fn load() -> Self {
        let settings = Ini::load_from_file(Self::SETTINGS_FILE_NAME)
            .unwrap_or_else(|_| Self::generate_default_config());
        Self {
            settings: Mutex::new(settings),
        }
    }

    fn generate_default_config() -> Ini {
        let mut settings = Ini::new();
        settings
            .with_section(Some("video"))
            .set("fullscreen", "true")
            .set("width", "")
            .set("height", "");
        settings.with_section(Some("audio"));
        settings
            .write_to_file(Self::SETTINGS_FILE_NAME)
            .unwrap_or_else(|_| {
                tracing::error!(
                    "Couldn't write default config file to {}",
                    Self::SETTINGS_FILE_NAME
                );
            });
        settings
    }

    fn get<S, K>(&self, section: Option<S>, key: K) -> Option<String>
    where
        S: Into<String>,
        K: AsRef<str>,
    {
        self.settings
            .lock()
            .unwrap()
            .section(section)?
            .get(key)
            .map(|s| s.to_owned())
    }

    fn set<S, K, V>(&self, section: Option<S>, key: K, value: V)
    where
        S: Into<String>,
        K: Into<String>,
        V: Into<String>,
    {
        let mut settings = self.settings.lock().unwrap();
        settings.with_section(section).set(key, value);
        settings
            .write_to_file(Self::SETTINGS_FILE_NAME)
            .unwrap_or_else(|_| {
                tracing::error!("Couldn't write config file to {}", Self::SETTINGS_FILE_NAME);
            });
    }

    pub fn get_bool<S, K>(&self, section: S, key: K, default: bool) -> bool
    where
        S: Into<String>,
        K: AsRef<str>,
    {
        match self.get(Some(section), key) {
            Some(s) if s.eq_ignore_ascii_case("true") => true,
            Some(s) if s.eq_ignore_ascii_case("false") => false,
            _ => default,
        }
    }

    pub fn set_bool<S, K>(&self, section: S, key: K, value: bool)
    where
        S: Into<String>,
        K: Into<String>,
    {
        self.set(Some(section), key, if value { "true" } else { "false" });
    }

    pub fn get_int<S, K>(&self, section: S, key: K, default: i32) -> i32
    where
        S: Into<String>,
        K: AsRef<str>,
    {
        self.get(Some(section), key)
            .and_then(|w| w.parse().ok())
            .unwrap_or(default)
    }
}
