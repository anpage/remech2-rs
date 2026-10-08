use std::{
    fs,
    io::ErrorKind,
    sync::{Arc, LazyLock, Mutex},
};

use serde::{Deserialize, Serialize};
use toml_edit::{DocumentMut, Item, Table, TableLike, Value, de::from_document, ser::to_document};
use tracing::{Level, error, warn};

use crate::{drawmode::ScalingMode, files, input::store::DEFAULT_PROFILE};

const FILE_NAME: &str = "remech2.toml";

#[derive(Clone, PartialEq, Default, Serialize, Deserialize)]
#[serde(default)]
pub struct Settings {
    pub video: VideoSettings,
    pub audio: AudioSettings,
    pub input: InputSettings,
    pub debug: DebugSettings,
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct VideoSettings {
    pub fullscreen: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub window_width: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub window_height: Option<u32>,
    pub widescreen: bool,
    pub framerate_limit: u32,
    pub scaling: ScalingMode,
}

impl Default for VideoSettings {
    fn default() -> Self {
        Self {
            fullscreen: true,
            window_width: None,
            window_height: None,
            widescreen: false,
            framerate_limit: 60,
            scaling: ScalingMode::default(),
        }
    }
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct AudioSettings {
    pub music_path: String,
}

impl Default for AudioSettings {
    fn default() -> Self {
        Self {
            music_path: "Music".to_owned(),
        }
    }
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct InputSettings {
    pub profile: String,
}

impl Default for InputSettings {
    fn default() -> Self {
        Self {
            profile: DEFAULT_PROFILE.to_owned(),
        }
    }
}

#[derive(Clone, PartialEq, Default, Serialize, Deserialize)]
#[serde(default)]
pub struct DebugSettings {
    pub log_level: LogLevel,
}

#[derive(Clone, Copy, PartialEq, Default, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum LogLevel {
    Trace,
    Debug,
    Info,
    #[default]
    Warn,
    Error,
}

impl From<LogLevel> for Level {
    fn from(level: LogLevel) -> Self {
        match level {
            LogLevel::Trace => Level::TRACE,
            LogLevel::Debug => Level::DEBUG,
            LogLevel::Info => Level::INFO,
            LogLevel::Warn => Level::WARN,
            LogLevel::Error => Level::ERROR,
        }
    }
}

enum Problem {
    Error(String),
    Warning(String),
}

struct Store {
    current: Arc<Settings>,
    document: DocumentMut,
    writable: bool,
    problems: Vec<Problem>,
}

static STORE: LazyLock<Mutex<Store>> = LazyLock::new(|| Mutex::new(Store::load()));

pub fn get() -> Arc<Settings> {
    STORE.lock().unwrap().current.clone()
}

pub fn update(change: impl FnOnce(&mut Settings)) {
    let mut store = STORE.lock().unwrap();
    let mut settings = (*store.current).clone();
    change(&mut settings);
    if settings == *store.current {
        return;
    }
    store.save(&settings);
    store.current = Arc::new(settings);
}

pub fn log_load_problems() {
    for problem in std::mem::take(&mut STORE.lock().unwrap().problems) {
        match problem {
            Problem::Error(message) => error!("{message}"),
            Problem::Warning(message) => warn!("{message}"),
        }
    }
}

impl Store {
    fn load() -> Self {
        let defaults = Self {
            current: Arc::default(),
            document: default_document(),
            writable: true,
            problems: Vec::new(),
        };
        let unwritable = |message: String| Self {
            current: Arc::default(),
            document: DocumentMut::new(),
            writable: false,
            problems: vec![Problem::Error(message)],
        };

        let path = files::resolve(FILE_NAME);
        let text = match fs::read_to_string(&path) {
            Ok(text) => text,
            Err(e) if e.kind() == ErrorKind::NotFound => {
                let mut store = defaults;
                if let Err(e) = store.write() {
                    store
                        .problems
                        .push(Problem::Error(format!("settings: {e:#}")));
                }
                return store;
            }
            Err(e) => {
                return unwritable(format!("settings: couldn't read {}: {e}", path.display()));
            }
        };
        let document: DocumentMut = match text.parse() {
            Ok(document) => document,
            Err(e) => {
                return unwritable(format!(
                    "settings: {} isn't valid TOML, so the defaults are used and it won't be saved: {e}",
                    path.display()
                ));
            }
        };

        let (current, problems) = overlay(&document);
        Self {
            current: Arc::new(current),
            document,
            writable: true,
            problems,
        }
    }

    fn save(&mut self, settings: &Settings) {
        if !self.writable {
            warn!("settings: not saving because {FILE_NAME} couldn't be read");
            return;
        }
        let (Ok(old), Ok(new)) = (to_document(&*self.current), to_document(settings)) else {
            error!("settings: couldn't serialize the settings");
            return;
        };

        for (section, new_section) in new.iter() {
            let Some(new_section) = new_section.as_table_like() else {
                continue;
            };
            let old_section = old.get(section).and_then(Item::as_table_like);
            let Some(file_section) = self
                .document
                .entry(section)
                .or_insert(Item::Table(Table::new()))
                .as_table_like_mut()
            else {
                continue;
            };

            for (key, value) in new_section.iter() {
                let Some(value) = value.as_value() else {
                    continue;
                };
                let old_value = old_section
                    .and_then(|old| old.get(key))
                    .and_then(Item::as_value);
                if old_value.map(Value::to_string) != Some(value.to_string()) {
                    set(file_section, key, value.clone());
                }
            }
            // An optional value that was unset
            for (key, _) in old_section.into_iter().flat_map(|old| old.iter()) {
                if !new_section.contains_key(key) {
                    file_section.remove(key);
                }
            }
        }

        if let Err(e) = self.write() {
            error!("settings: {e:#}");
        }
    }

    fn write(&self) -> anyhow::Result<()> {
        files::write_atomically(&files::resolve_user(FILE_NAME), &self.document.to_string())
    }
}

fn default_document() -> DocumentMut {
    let mut document = to_document(&Settings::default()).expect("the defaults serialize");
    for (_, item) in document.as_table_mut().iter_mut() {
        let section = std::mem::take(item);
        *item = section.into_table().map_or_else(|other| other, Item::Table);
    }
    format!("{document}").parse().expect("the defaults parse")
}

fn overlay(document: &DocumentMut) -> (Settings, Vec<Problem>) {
    let mut merged = to_document(&Settings::default()).expect("the defaults serialize");
    let mut settings = Settings::default();
    let mut problems = Vec::new();

    for (section, item) in document.iter() {
        let Some(table) = item.as_table_like() else {
            continue;
        };
        for (key, item) in table.iter() {
            let Some(value) = item.as_value() else {
                continue;
            };
            let Some(merged_section) = merged.get_mut(section).and_then(Item::as_table_like_mut)
            else {
                continue;
            };
            let previous = merged_section.insert(key, Item::Value(value.clone()));

            match from_document::<Settings>(merged.clone()) {
                Ok(parsed) => settings = parsed,
                Err(e) => {
                    problems.push(Problem::Warning(format!(
                        "settings: {section}.{key} = {} isn't valid, using its default: {}",
                        value.to_string().trim(),
                        e.message()
                    )));
                    let merged_section = merged
                        .get_mut(section)
                        .and_then(Item::as_table_like_mut)
                        .unwrap();
                    match previous {
                        Some(previous) => merged_section.insert(key, previous),
                        None => merged_section.remove(key),
                    };
                }
            }
        }
    }
    (settings, problems)
}

fn set(table: &mut dyn TableLike, key: &str, mut value: Value) {
    match table.get_mut(key).and_then(Item::as_value_mut) {
        Some(old) => {
            *value.decor_mut() = old.decor().clone();
            *old = value;
        }
        None => {
            table.insert(key, Item::Value(value));
        }
    }
}
