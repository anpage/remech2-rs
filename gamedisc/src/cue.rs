//! Parser for cue sheet files

use std::path::Path;

use winnow::{
    ascii::{Caseless, digit1, space1},
    combinator::{alt, cut_err, delimited, eof, opt, peek, preceded, separated_pair},
    prelude::*,
    token::{rest, take_till, take_while},
};

#[derive(Debug, thiserror::Error)]
pub enum CueError {
    #[error("line {line}: track {track} has no INDEX 01")]
    MissingIndex01 { line: usize, track: u8 },
    #[error("line {line}: TRACK before any FILE")]
    TrackBeforeFile { line: usize },
    #[error("line {line}: unsupported TRACK mode {mode}")]
    UnsupportedMode { line: usize, mode: String },
    #[error("line {line}: INDEX goes backwards")]
    IndexOrder { line: usize },
    #[error("line {line}: invalid syntax ({text})")]
    Syntax { line: usize, text: String },
    #[error("line {line}: unsupported FILE type {kind}")]
    UnsupportedFileType { line: usize, kind: String },
    #[error("line {line}: INDEX before any TRACK")]
    IndexBeforeTrack { line: usize },
    #[error("line {line}: FILE has no TRACKs")]
    FileMissingTracks { line: usize },
    #[error("CUE sheet has no FILEs")]
    MissingFiles,
    #[error("line {line}: TRACK {number} has a data mode in an audio FILE")]
    DataModeInAudioFile { line: usize, number: u8 },
    #[error("line {line}: TRACK {number} has an invalid number (1-99)")]
    InvalidTrackNumber { line: usize, number: u8 },
    #[error("line {line}: TRACK {number} is out of order")]
    TrackOrder { line: usize, number: u8 },
    #[error("line {line}: INDEX {index} is a duplicate")]
    DuplicateIndex { line: usize, index: u8 },
}

/// A parsed cue sheet with its listed files and their tracks
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CueSheet {
    pub files: Vec<CueFile>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CueFile {
    /// The file's path as written in the sheet
    pub path: String,
    /// Either `BINARY` or one of the supported audio file types
    pub kind: CueFileKind,
    /// Never empty
    pub tracks: Vec<CueTrack>,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CueFileKind {
    /// Raw sectors with a shape defined by `CueMode`
    Binary,
    Wave,
    Mp3,
    Ogg,
    Flac,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CueTrack {
    /// 1 to 99
    pub number: u8,
    pub mode: CueMode,
    pub index00: Option<u32>,
    pub index01: u32,
}

/// Sector layout inside a `BINARY`-type file
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CueMode {
    /// 2352-byte sectors of 16-bit stereo PCM
    Audio,
    /// 2352-byte sectors with 2048 bytes of user data at offset 16
    Mode1Raw,
    /// 2048-byte sectors of user data only
    Mode1Cooked,
    /// 2352-byte XA sectors with user data at offset 24
    Mode2Raw,
}

enum Command<'a> {
    File {
        name: &'a str,
        kind: Option<&'a str>,
    },
    Track {
        number: u8,
        mode: &'a str,
    },
    Index {
        number: u8,
        frames: u32,
    },
    Other,
}

fn number<T: std::str::FromStr>(i: &mut &str) -> ModalResult<T> {
    digit1.parse_to().parse_next(i)
}

fn name<'a>(i: &mut &'a str) -> ModalResult<&'a str> {
    alt((
        delimited('"', take_till(1.., '"'), '"'),
        take_while(1.., |c: char| !c.is_whitespace() && c != '"'),
    ))
    .parse_next(i)
}

/// mm:ss:ff to sectors
fn msf(i: &mut &str) -> ModalResult<u32> {
    let (m, (s, f)): (u32, (u32, u32)) =
        separated_pair(number, ':', separated_pair(number, ':', number))
            .verify(|(m, (s, f)): &(u32, (u32, u32))| *m < 100 && *s < 60 && *f < 75)
            .parse_next(i)?;
    Ok((m * 60 + s) * 75 + f)
}

fn command<'a>(i: &mut &'a str) -> ModalResult<Command<'a>> {
    alt((
        preceded(
            (Caseless("FILE"), peek(alt((space1, eof)))),
            cut_err(preceded(space1, (name, opt(preceded(space1, name))))),
        )
        .map(|(name, kind)| Command::File { name, kind }),
        preceded(
            (Caseless("TRACK"), peek(alt((space1, eof)))),
            cut_err(preceded(space1, separated_pair(number, space1, name))),
        )
        .map(|(number, mode)| Command::Track { number, mode }),
        preceded(
            (Caseless("INDEX"), peek(alt((space1, eof)))),
            cut_err(preceded(space1, separated_pair(number, space1, msf))),
        )
        .map(|(number, frames)| Command::Index { number, frames }),
        rest.map(|_| Command::Other),
    ))
    .parse_next(i)
}

struct PendingFile {
    line: usize,
    path: String,
    kind: CueFileKind,
    tracks: Vec<CueTrack>,
}

impl PendingFile {
    fn new(line: usize, path: String, kind: CueFileKind) -> Self {
        Self {
            line,
            path,
            kind,
            tracks: vec![],
        }
    }

    fn finish(self) -> Result<CueFile, CueError> {
        if self.tracks.is_empty() {
            return Err(CueError::FileMissingTracks { line: self.line });
        }

        Ok(CueFile {
            path: self.path,
            kind: self.kind,
            tracks: self.tracks,
        })
    }
}

struct PendingTrack {
    line: usize,
    number: u8,
    mode: CueMode,
    index00: Option<u32>,
    index01: Option<u32>,
}

impl PendingTrack {
    fn new(line: usize, number: u8, mode: CueMode) -> Self {
        Self {
            line,
            number,
            mode,
            index00: None,
            index01: None,
        }
    }

    fn finish(self) -> Result<CueTrack, CueError> {
        let index01 = self.index01.ok_or(CueError::MissingIndex01 {
            line: self.line,
            track: self.number,
        })?;

        if let Some(index00) = self.index00
            && index00 > index01
        {
            return Err(CueError::IndexOrder { line: self.line });
        }

        Ok(CueTrack {
            number: self.number,
            mode: self.mode,
            index00: self.index00,
            index01,
        })
    }
}

fn flush_track(file: &mut PendingFile, track: &mut Option<PendingTrack>) -> Result<(), CueError> {
    let Some(t) = track.take() else {
        return Ok(());
    };

    let line = t.line;
    let t = t.finish()?;

    if let Some(prev) = file.tracks.last()
        && t.index00.unwrap_or(t.index01) < prev.index01
    {
        return Err(CueError::IndexOrder { line });
    }

    if !matches!(t.mode, CueMode::Audio) && !matches!(file.kind, CueFileKind::Binary) {
        return Err(CueError::DataModeInAudioFile {
            line,
            number: t.number,
        });
    }

    file.tracks.push(t);

    Ok(())
}

fn extension_to_kind(line: usize, path: &str) -> Result<CueFileKind, CueError> {
    let file_name = path.rsplit(['/', '\\']).next().unwrap_or_default();
    let Some(extension) = Path::new(file_name).extension() else {
        return Err(CueError::UnsupportedFileType {
            line,
            kind: String::from("(no extension)"),
        });
    };

    let extension = extension.to_string_lossy().to_ascii_lowercase();

    match extension.as_str() {
        "bin" | "img" | "iso" => Ok(CueFileKind::Binary),
        "wav" => Ok(CueFileKind::Wave),
        "mp3" => Ok(CueFileKind::Mp3),
        "ogg" => Ok(CueFileKind::Ogg),
        "flac" => Ok(CueFileKind::Flac),
        _ => Err(CueError::UnsupportedFileType {
            line,
            kind: extension,
        }),
    }
}

impl CueSheet {
    /// Parses a cue sheet.
    ///
    /// # Errors
    ///
    /// Returns the first problem found with its line number.
    pub fn parse(text: &str) -> Result<Self, CueError> {
        let mut files = vec![];

        let mut current_file: Option<PendingFile> = None;
        let mut current_track: Option<PendingTrack> = None;
        let mut last_track_number: Option<u8> = None;

        for (line_number, line) in text
            .strip_prefix('\u{feff}')
            .unwrap_or(text)
            .lines()
            .enumerate()
        {
            let line_number = line_number + 1;

            let Ok(command) = command.parse(line.trim()) else {
                return Err(CueError::Syntax {
                    line: line_number,
                    text: line.trim().to_owned(),
                });
            };

            match command {
                Command::File { name, kind } => {
                    if let Some(mut file) = current_file {
                        let carried = match &mut current_track {
                            Some(t) if t.index01.is_none() && t.index00.is_some() => {
                                t.index00 = None;
                                true
                            }
                            _ => {
                                flush_track(&mut file, &mut current_track)?;
                                false
                            }
                        };
                        if !carried || !file.tracks.is_empty() {
                            files.push(file.finish()?);
                        }
                    }

                    let kind = if let Some(kind) = kind {
                        match kind.to_ascii_uppercase().as_str() {
                            "BINARY" => CueFileKind::Binary,
                            "MOTOROLA" => {
                                return Err(CueError::UnsupportedFileType {
                                    line: line_number,
                                    kind: kind.to_owned(),
                                });
                            }
                            _ => extension_to_kind(line_number, name)?,
                        }
                    } else {
                        extension_to_kind(line_number, name)?
                    };

                    current_file = Some(PendingFile::new(line_number, name.to_owned(), kind));
                }
                Command::Track { number, mode } => {
                    let Some(file) = &mut current_file else {
                        return Err(CueError::TrackBeforeFile { line: line_number });
                    };

                    flush_track(file, &mut current_track)?;

                    if !(1..=99).contains(&number) {
                        return Err(CueError::InvalidTrackNumber {
                            line: line_number,
                            number,
                        });
                    }

                    if let Some(last) = last_track_number
                        && number != last + 1
                    {
                        return Err(CueError::TrackOrder {
                            line: line_number,
                            number,
                        });
                    }
                    last_track_number = Some(number);

                    let mode = match mode.to_ascii_uppercase().as_str() {
                        "AUDIO" => CueMode::Audio,
                        "MODE1/2352" => CueMode::Mode1Raw,
                        "MODE1/2048" => CueMode::Mode1Cooked,
                        "MODE2/2352" => CueMode::Mode2Raw,
                        _ => {
                            return Err(CueError::UnsupportedMode {
                                line: line_number,
                                mode: mode.to_owned(),
                            });
                        }
                    };

                    current_track = Some(PendingTrack::new(line_number, number, mode));
                }
                Command::Index { number, frames } => {
                    let Some(track) = &mut current_track else {
                        return Err(CueError::IndexBeforeTrack { line: line_number });
                    };

                    match number {
                        0 => {
                            if track.index00.is_some() {
                                return Err(CueError::DuplicateIndex {
                                    line: line_number,
                                    index: 0,
                                });
                            }
                            track.index00 = Some(frames);
                        }
                        1 => {
                            if track.index01.is_some() {
                                return Err(CueError::DuplicateIndex {
                                    line: line_number,
                                    index: 1,
                                });
                            }
                            track.index01 = Some(frames);
                        }
                        _ => {}
                    }
                }
                _ => {}
            }
        }

        if let Some(mut file) = current_file {
            flush_track(&mut file, &mut current_track)?;
            files.push(file.finish()?);
        }

        if files.is_empty() {
            return Err(CueError::MissingFiles);
        }

        Ok(Self { files })
    }
}
