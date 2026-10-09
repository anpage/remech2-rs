use std::{
    fs,
    io::ErrorKind,
    sync::{Arc, LazyLock, Mutex},
};

use remech2_sys::shared;
use serde::{Deserialize, Serialize};
use toml_edit::{DocumentMut, Item, Table, TableLike, Value, de::from_document, ser::to_document};
use tracing::{Level as TracingLevel, error, warn};

use crate::{
    drawmode::ScalingMode, files, input::store::DEFAULT_PROFILE, resolution::RenderResolution,
};

const FILE_NAME: &str = "remech2.toml";

#[derive(Clone, PartialEq, Default, Serialize, Deserialize)]
#[serde(default)]
pub struct Settings {
    pub video: VideoSettings,
    pub audio: AudioSettings,
    pub difficulty: DifficultySettings,
    pub input: InputSettings,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub debug: Option<DebugSettings>,
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct VideoSettings {
    pub fullscreen: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub window_width: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub window_height: Option<u32>,
    /// The game's internal resolution
    pub render_resolution: RenderResolution,
    pub widescreen: bool,
    pub vsync: bool,
    pub framerate_limit: u32,
    pub scaling: ScalingMode,
    pub object_textmaps: bool,
    pub terrain_textmaps: bool,
    pub detail: Level,
    pub object_density: Level,
    pub explosion_chunks: bool,
    pub brightness: UpTo<15>,
}

impl Default for VideoSettings {
    fn default() -> Self {
        Self {
            fullscreen: true,
            window_width: None,
            window_height: None,
            render_resolution: Default::default(),
            widescreen: false,
            vsync: true,
            framerate_limit: 180,
            scaling: Default::default(),
            object_textmaps: true,
            terrain_textmaps: true,
            detail: Level::High,
            object_density: Level::High,
            explosion_chunks: true,
            brightness: UpTo(9),
        }
    }
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct AudioSettings {
    pub music_path: String,
    pub effects_volume: UpTo<100>,
    pub voice_volume: UpTo<100>,
    pub music_volume: UpTo<100>,
}

impl Default for AudioSettings {
    fn default() -> Self {
        Self {
            music_path: "Music".to_owned(),
            effects_volume: UpTo(100),
            voice_volume: UpTo(100),
            music_volume: UpTo(100),
        }
    }
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct DifficultySettings {
    pub enemy_skill: Skill,
    pub unlimited_ammo: bool,
    pub invulnerable: bool,
    pub splash_damage: bool,
    pub collision_damage: bool,
    pub heat_tracking: bool,
}

impl Default for DifficultySettings {
    fn default() -> Self {
        Self {
            enemy_skill: Skill::Medium,
            unlimited_ammo: false,
            invulnerable: false,
            splash_damage: true,
            collision_damage: true,
            heat_tracking: true,
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
    #[serde(skip_serializing_if = "Option::is_none")]
    pub log_level: Option<LogLevel>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub log_file: Option<String>,
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

impl From<LogLevel> for TracingLevel {
    fn from(level: LogLevel) -> Self {
        match level {
            LogLevel::Trace => TracingLevel::TRACE,
            LogLevel::Debug => TracingLevel::DEBUG,
            LogLevel::Info => TracingLevel::INFO,
            LogLevel::Warn => TracingLevel::WARN,
            LogLevel::Error => TracingLevel::ERROR,
        }
    }
}

#[derive(Clone, Copy, PartialEq, Default, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum Level {
    Low,
    #[default]
    High,
}

#[derive(Clone, Copy, PartialEq, Default, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum Skill {
    Easy,
    #[default]
    Medium,
    Hard,
}

/// A whole number from 0 to `MAX`
#[derive(Clone, Copy, PartialEq, Debug, Serialize, Deserialize)]
#[serde(try_from = "u8", into = "u8")]
pub struct UpTo<const MAX: u8>(u8);

impl<const MAX: u8> UpTo<MAX> {
    pub fn get(self) -> u8 {
        self.0
    }

    /// `value`, clamped to 0 to `MAX`
    pub fn saturating(value: i32) -> Self {
        Self(value.clamp(0, i32::from(MAX)) as u8)
    }
}

impl<const MAX: u8> TryFrom<u8> for UpTo<MAX> {
    type Error = String;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        if value <= MAX {
            Ok(Self(value))
        } else {
            Err(format!("{value} is more than {MAX}"))
        }
    }
}

impl<const MAX: u8> From<UpTo<MAX>> for u8 {
    fn from(value: UpTo<MAX>) -> Self {
        value.0
    }
}

/// A volume percentage in the game's fixed point format
fn volume_fraction(percent: UpTo<100>) -> i32 {
    (i32::from(percent.get()) << 16) / 100
}

/// A fixed point volume value as a rounded percentage
fn volume_percent(fraction: i32) -> UpTo<100> {
    UpTo::saturating((fraction.clamp(0, 0x10000) * 100 + 0x8000) >> 16)
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
    merged.insert("debug", Item::Table(Table::new()));
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

#[unsafe(export_name = "MechGetSoundSettings")]
pub unsafe extern "C" fn get_sound_settings(out: *mut shared::SoundSettings) {
    let Some(out) = (unsafe { out.as_mut() }) else {
        return;
    };

    let settings = get();
    let audio = &settings.audio;

    *out = shared::SoundSettings {
        m_effectsVolume: volume_fraction(audio.effects_volume),
        m_voiceVolume: volume_fraction(audio.voice_volume),
        m_musicVolume: volume_fraction(audio.music_volume),
    };
}

#[unsafe(export_name = "MechSetSoundSettings")]
pub unsafe extern "C" fn set_sound_settings(new: *const shared::SoundSettings) {
    let Some(new) = (unsafe { new.as_ref() }) else {
        return;
    };

    update(|settings| {
        let audio = &mut settings.audio;
        audio.effects_volume = volume_percent(new.m_effectsVolume);
        audio.voice_volume = volume_percent(new.m_voiceVolume);
        audio.music_volume = volume_percent(new.m_musicVolume);
    });
}

#[unsafe(export_name = "MechGetDisplaySettings")]
pub unsafe extern "C" fn get_display_settings(out: *mut shared::DisplaySettings) {
    let Some(out) = (unsafe { out.as_mut() }) else {
        return;
    };

    let settings = get();
    let video = &settings.video;

    *out = shared::DisplaySettings {
        m_objectTextmaps: video.object_textmaps.into(),
        m_terrainTextmaps: video.terrain_textmaps.into(),
        m_highDetail: (video.detail == Level::High).into(),
        m_highObjectDensity: (video.object_density == Level::High).into(),
        m_explosionChunks: video.explosion_chunks.into(),
        m_brightness: video.brightness.get().into(),
    };
}

#[unsafe(export_name = "MechSetDisplaySettings")]
pub unsafe extern "C" fn set_display_settings(new: *const shared::DisplaySettings) {
    let Some(new) = (unsafe { new.as_ref() }) else {
        return;
    };

    let level = |high: i32| if high != 0 { Level::High } else { Level::Low };

    update(|settings| {
        let video = &mut settings.video;
        video.object_textmaps = new.m_objectTextmaps != 0;
        video.terrain_textmaps = new.m_terrainTextmaps != 0;
        video.detail = level(new.m_highDetail);
        video.object_density = level(new.m_highObjectDensity);
        video.explosion_chunks = new.m_explosionChunks != 0;
        video.brightness = UpTo::saturating(new.m_brightness);
    });
}

#[unsafe(export_name = "MechGetDifficultySettings")]
pub unsafe extern "C" fn get_difficulty_settings(out: *mut shared::DifficultySettings) {
    let Some(out) = (unsafe { out.as_mut() }) else {
        return;
    };
    let settings = get();
    let difficulty = &settings.difficulty;
    *out = shared::DifficultySettings {
        m_unlimitedAmmo: difficulty.unlimited_ammo.into(),
        m_invulnerable: difficulty.invulnerable.into(),
        m_splashDamage: difficulty.splash_damage.into(),
        m_collisionDamage: difficulty.collision_damage.into(),
        m_heatTracking: difficulty.heat_tracking.into(),
        m_enemySkill: difficulty.enemy_skill as u8,
    };
}

#[unsafe(export_name = "MechSetDifficultySettings")]
pub unsafe extern "C" fn set_difficulty_settings(new: *const shared::DifficultySettings) {
    let Some(new) = (unsafe { new.as_ref() }) else {
        return;
    };
    let skill = match new.m_enemySkill {
        0 => Skill::Easy,
        2 => Skill::Hard,
        _ => Skill::Medium,
    };
    update(|settings| {
        settings.difficulty = DifficultySettings {
            enemy_skill: skill,
            unlimited_ammo: new.m_unlimitedAmmo != 0,
            invulnerable: new.m_invulnerable != 0,
            splash_damage: new.m_splashDamage != 0,
            collision_damage: new.m_collisionDamage != 0,
            heat_tracking: new.m_heatTracking != 0,
        }
    });
}
