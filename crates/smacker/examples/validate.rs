//! Corpus validation harness.
//!
//! Decodes every frame of every `.smk` file below the given paths and checks
//! the four properties that hold for a correct decoder:
//!
//! 1. every file opens and every frame decodes without error;
//! 2. the video bitstream ends 0–4 bytes short of the end of its sub-chunk
//!    (frame chunks are dword-aligned, so a correct decoder lands there while
//!    a desynchronised one drifts);
//! 3. every audio sub-chunk likewise ends within 4 bytes;
//! 4. every palette record completes 256 entries within its own declared
//!    length and no component exceeds `0x3F`.
//!
//! It also checks that the blit never writes outside the destination's
//! `width` columns, that decoded audio fits the header's per-track maximum,
//! and prints a summary of the corpus shape (dimensions, frame times, audio
//! configurations, keyframes) so unexpected inputs stand out.
//!
//! ```text
//! cargo run --release --example validate --target <host triple> -- [OPTIONS] [PATH...]
//! ```
//!
//! With no `PATH`, the directory named by `SMACKER_CORPUS` is used, falling
//! back to `data/SMK` at the repository root. Exit status is 0 when every
//! check passes, 1 when a check fails, and 2 when the harness could not run
//! (bad arguments, unreadable path, no input files).

#![forbid(unsafe_code)]

use std::collections::BTreeSet;
use std::path::{Path, PathBuf};
use std::process::ExitCode;

use smacker::{
    AudioDecoder, AudioInfo, DecodedAudio, Error, FRAME_TYPE_PALETTE, Header, HuffmanTables,
    Palette, TRACK_COUNT, VideoDecoder, split_record,
};

/// Extra destination columns beyond the frame width, so a blit that writes
/// past the frame is caught.
const PAD_COLUMNS: usize = 8;
/// Fill value for those columns; the blit must never touch them.
const PAD_FILL: u8 = 0xCD;
/// Largest trailing gap allowed between the end of a bitstream and the end of
/// the sub-chunk that holds it (dword padding).
const MAX_PADDING: usize = 4;
/// Findings reported per file before the rest are only counted.
const MAX_REPORTED: usize = 8;

fn main() -> ExitCode {
    let mut verbose = false;
    let mut roots: Vec<PathBuf> = Vec::new();
    for arg in std::env::args().skip(1) {
        match arg.as_str() {
            "-v" | "--verbose" => verbose = true,
            "-h" | "--help" => {
                print_usage();
                return ExitCode::SUCCESS;
            }
            other if other.starts_with('-') => {
                eprintln!("validate: unknown option `{other}`");
                print_usage();
                return ExitCode::from(2);
            }
            other => roots.push(PathBuf::from(other)),
        }
    }
    if roots.is_empty() {
        roots.push(default_corpus());
    }

    let mut files = Vec::new();
    for root in &roots {
        if let Err(e) = collect(root, &mut files) {
            eprintln!("validate: {}: {e}", root.display());
            return ExitCode::from(2);
        }
    }
    files.sort();
    files.dedup();
    if files.is_empty() {
        eprintln!(
            "validate: no .smk files under {}",
            roots
                .iter()
                .map(|p| p.display().to_string())
                .collect::<Vec<_>>()
                .join(", ")
        );
        return ExitCode::from(2);
    }

    let mut totals = Totals::default();
    for path in &files {
        let (stats, findings) = check_file(path);
        let name = path.file_name().map_or_else(
            || path.display().to_string(),
            |n| n.to_string_lossy().into_owned(),
        );
        if findings.is_empty() {
            if verbose {
                println!("ok   {name}: {}", stats.summary());
            }
        } else {
            println!("FAIL {name}: {}", stats.summary());
            for item in &findings.items {
                println!("       {item}");
            }
            if findings.total > findings.items.len() {
                println!(
                    "       … and {} more",
                    findings.total - findings.items.len()
                );
            }
        }
        totals.add(&stats, findings.total);
    }

    totals.report();
    if totals.failed_files == 0 {
        ExitCode::SUCCESS
    } else {
        ExitCode::FAILURE
    }
}

fn print_usage() {
    println!(
        "usage: validate [-v|--verbose] [PATH...]\n\
         \n\
         Decodes every .smk file below each PATH and checks that every frame,\n\
         audio sub-chunk and palette record decodes cleanly and lands where a\n\
         correct decoder lands. Defaults to $SMACKER_CORPUS, then data/SMK."
    );
}

fn default_corpus() -> PathBuf {
    std::env::var_os("SMACKER_CORPUS").map_or_else(
        || PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../data/SMK"),
        PathBuf::from,
    )
}

/// Collect `.smk` files at `path`, descending into directories.
fn collect(path: &Path, out: &mut Vec<PathBuf>) -> std::io::Result<()> {
    if path.is_dir() {
        let mut entries: Vec<PathBuf> = std::fs::read_dir(path)?
            .collect::<std::io::Result<Vec<_>>>()?
            .into_iter()
            .map(|e| e.path())
            .collect();
        entries.sort();
        for entry in entries {
            collect(&entry, out)?;
        }
        return Ok(());
    }
    if !path.exists() {
        return Err(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            "no such file or directory",
        ));
    }
    let is_smk = path
        .extension()
        .is_some_and(|e| e.eq_ignore_ascii_case("smk"));
    if is_smk {
        out.push(path.to_path_buf());
    }
    Ok(())
}

/// Problems found in one file. Only the first [`MAX_REPORTED`] are kept; the
/// rest are counted so a desynchronised decoder does not flood the report.
#[derive(Default)]
struct Findings {
    items: Vec<String>,
    total: usize,
}

impl Findings {
    fn push(&mut self, item: String) {
        self.total += 1;
        if self.items.len() < MAX_REPORTED {
            self.items.push(item);
        }
    }

    fn is_empty(&self) -> bool {
        self.total == 0
    }
}

/// What one file contributed to the corpus summary.
#[derive(Default)]
struct FileStats {
    width: u32,
    height: u32,
    frames: u64,
    frame_time: u32,
    keyframes: u64,
    max_chunk: usize,
    palette_records: u64,
    max_component: u8,
    audio_chunks: u64,
    audio_bytes: u64,
    tracks: Vec<AudioInfo>,
}

impl FileStats {
    fn summary(&self) -> String {
        let tracks = join(
            self.tracks
                .iter()
                .map(|t| format!("{}-bit {} {} Hz", t.bits, layout(t.channels), t.rate)),
        );
        let separator = if tracks.is_empty() { "" } else { ", " };
        format!(
            "{}x{}, {} frames, {} 10µs{separator}{tracks}",
            self.width, self.height, self.frames, self.frame_time
        )
    }
}

/// A present audio track: its decoder and a destination buffer sized from the
/// header's per-track maximum.
struct Track {
    decoder: AudioDecoder,
    dst: Vec<u8>,
    /// Whether the destination has had to grow past the header's maximum.
    grown: bool,
}

impl Track {
    /// Decode one sub-chunk, growing the destination if the header's declared
    /// maximum turns out to be too small (reported as a finding).
    fn decode(&mut self, sub: &[u8]) -> Result<(DecodedAudio, bool), Error> {
        match self.decoder.decode(sub, &mut self.dst) {
            Err(Error::OutputBufferTooSmall { needed, .. }) => {
                self.dst.resize(needed, 0);
                let grew = !self.grown;
                self.grown = true;
                self.decoder.decode(sub, &mut self.dst).map(|r| (r, grew))
            }
            other => other.map(|r| (r, false)),
        }
    }
}

fn check_file(path: &Path) -> (FileStats, Findings) {
    match std::fs::read(path) {
        Ok(bytes) => check_bytes(&bytes),
        Err(e) => {
            let mut findings = Findings::default();
            findings.push(format!("read failed: {e}"));
            (FileStats::default(), findings)
        }
    }
}

fn check_bytes(bytes: &[u8]) -> (FileStats, Findings) {
    let mut stats = FileStats::default();
    let mut findings = Findings::default();

    let header = match Header::parse(bytes) {
        Ok(h) => h,
        Err(e) => {
            findings.push(format!("open failed: {e}"));
            return (stats, findings);
        }
    };
    let info = header.video_info();
    stats.width = info.width;
    stats.height = info.height;
    stats.frame_time = info.frame_time.0;

    let tables = match HuffmanTables::parse(header.tree_data(), header.tree_sizes()) {
        Ok(t) => t,
        Err(e) => {
            findings.push(format!("tree parse failed: {e}"));
            return (stats, findings);
        }
    };

    let mut video = VideoDecoder::new(&info, tables);
    let mut palette = Palette::new();
    let mut tracks: Vec<Option<Track>> = Vec::with_capacity(TRACK_COUNT);
    for t in 0..TRACK_COUNT {
        let Some(track_info) = header.audio_track(t) else {
            tracks.push(None);
            continue;
        };
        stats.tracks.push(track_info);
        match AudioDecoder::new(track_info) {
            Ok(decoder) => tracks.push(Some(Track {
                decoder,
                dst: vec![0; track_info.max_unpacked_size as usize],
                grown: false,
            })),
            Err(e) => {
                findings.push(format!("track {t}: {e}"));
                tracks.push(None);
            }
        }
    }

    let width = info.width as usize;
    let height = info.height as usize;
    let pitch = width + PAD_COLUMNS;
    let mut dst = vec![PAD_FILL; pitch * height];

    for i in 0..header.table_entries() {
        if header.is_keyframe(i) {
            stats.keyframes += 1;
        }
        let chunk = match header.frame_chunk(bytes, i) {
            Ok(c) => c,
            Err(e) => {
                findings.push(format!("frame {i}: {e}"));
                break;
            }
        };
        stats.max_chunk = stats.max_chunk.max(chunk.len());

        if header.frame_type(i) & FRAME_TYPE_PALETTE != 0 {
            stats.palette_records += 1;
            check_palette(&mut palette, chunk, i, &mut stats, &mut findings);
        }

        for (t, track) in tracks.iter_mut().enumerate() {
            let Some(track) = track.as_mut() else {
                continue;
            };
            check_audio(&header, track, chunk, i, t, &mut stats, &mut findings);
        }

        let bits = match header.video_subchunk(chunk, i) {
            Ok(b) => b,
            Err(e) => {
                findings.push(format!("frame {i}: {e}"));
                continue;
            }
        };
        // Alternate the flip flag so both blit orientations are exercised.
        match video.decode_frame(bits, &mut dst, pitch, i % 2 == 0) {
            Ok(consumed) => {
                stats.frames += 1;
                match bits.len().checked_sub(consumed) {
                    Some(left) if left > MAX_PADDING => findings.push(format!(
                        "frame {i}: video bitstream ends {left} bytes before its sub-chunk end"
                    )),
                    Some(_) => {}
                    None => findings.push(format!(
                        "frame {i}: video decode read {} bytes past its {}-byte sub-chunk",
                        consumed - bits.len(),
                        bits.len()
                    )),
                }
                if let Some(row) = padding_overrun(&dst, pitch, width, height) {
                    findings.push(format!("frame {i}: blit wrote past the frame on row {row}"));
                    for row in 0..height {
                        dst[row * pitch + width..(row + 1) * pitch].fill(PAD_FILL);
                    }
                }
            }
            Err(e) => findings.push(format!("frame {i}: video decode failed: {e}")),
        }
    }

    (stats, findings)
}

fn check_palette(
    palette: &mut Palette,
    chunk: &[u8],
    frame: usize,
    stats: &mut FileStats,
    findings: &mut Findings,
) {
    let ops = match split_record(chunk) {
        Ok((ops, _)) => ops,
        Err(e) => {
            findings.push(format!("frame {frame}: palette record: {e}"));
            return;
        }
    };
    if let Err(e) = palette.apply_record(ops) {
        findings.push(format!("frame {frame}: palette record: {e}"));
        return;
    }
    let max = palette
        .current()
        .iter()
        .flatten()
        .copied()
        .max()
        .unwrap_or(0);
    stats.max_component = stats.max_component.max(max);
    if max > 0x3F {
        findings.push(format!(
            "frame {frame}: palette component {max:#04x} exceeds 0x3f"
        ));
    }
}

fn check_audio(
    header: &Header<'_>,
    track: &mut Track,
    chunk: &[u8],
    frame: usize,
    t: usize,
    stats: &mut FileStats,
    findings: &mut Findings,
) {
    let sub = match header.audio_subchunk(chunk, frame, t) {
        Ok(Some(sub)) => sub,
        Ok(None) => return,
        Err(e) => {
            findings.push(format!("frame {frame}: track {t}: {e}"));
            return;
        }
    };
    let (decoded, grew) = match track.decode(sub) {
        Ok(r) => r,
        Err(e) => {
            findings.push(format!(
                "frame {frame}: track {t}: audio decode failed: {e}"
            ));
            return;
        }
    };
    stats.audio_chunks += 1;
    stats.audio_bytes += decoded.bytes_written as u64;
    if grew {
        findings.push(format!(
            "frame {frame}: track {t}: decoded {} bytes, past the header's maximum",
            decoded.bytes_written
        ));
    }
    match sub.len().checked_sub(decoded.bytes_consumed) {
        Some(left) if left > MAX_PADDING => findings.push(format!(
            "frame {frame}: track {t}: audio bitstream ends {left} bytes before its sub-chunk end"
        )),
        Some(_) => {}
        None => findings.push(format!(
            "frame {frame}: track {t}: audio decode read {} bytes past its {}-byte sub-chunk",
            decoded.bytes_consumed - sub.len(),
            sub.len()
        )),
    }
}

/// The first destination row whose padding columns were disturbed, if any.
fn padding_overrun(dst: &[u8], pitch: usize, width: usize, height: usize) -> Option<usize> {
    (0..height).find(|&row| {
        dst.get(row * pitch + width..(row + 1) * pitch)
            .is_none_or(|pad| pad.iter().any(|&b| b != PAD_FILL))
    })
}

/// Corpus-wide tallies.
#[derive(Default)]
struct Totals {
    files: usize,
    failed_files: usize,
    findings: usize,
    frames: u64,
    keyframes: u64,
    max_chunk: usize,
    palette_records: u64,
    max_component: u8,
    audio_files: usize,
    audio_chunks: u64,
    audio_bytes: u64,
    dimensions: BTreeSet<(u32, u32)>,
    frame_times: BTreeSet<u32>,
    audio_configs: BTreeSet<(u8, u8, u32)>,
}

impl Totals {
    fn add(&mut self, stats: &FileStats, findings: usize) {
        self.files += 1;
        if findings > 0 {
            self.failed_files += 1;
            self.findings += findings;
        }
        self.frames += stats.frames;
        self.keyframes += stats.keyframes;
        self.max_chunk = self.max_chunk.max(stats.max_chunk);
        self.palette_records += stats.palette_records;
        self.max_component = self.max_component.max(stats.max_component);
        self.audio_chunks += stats.audio_chunks;
        self.audio_bytes += stats.audio_bytes;
        if !stats.tracks.is_empty() {
            self.audio_files += 1;
        }
        if stats.width != 0 {
            self.dimensions.insert((stats.width, stats.height));
            self.frame_times.insert(stats.frame_time);
        }
        for track in &stats.tracks {
            self.audio_configs
                .insert((track.bits, track.channels, track.rate));
        }
    }

    fn report(&self) {
        let dims = join(self.dimensions.iter().map(|&(w, h)| format!("{w}x{h}")));
        let times = join(self.frame_times.iter().map(u32::to_string));
        let configs =
            join(self.audio_configs.iter().map(|&(bits, channels, rate)| {
                format!("{bits}-bit {} {rate} Hz", layout(channels))
            }));

        println!();
        println!(
            "files       {} checked, {} ok, {} failed ({} findings)",
            self.files,
            self.files - self.failed_files,
            self.failed_files,
            self.findings
        );
        println!(
            "frames      {} decoded, {} keyframes, largest chunk {} bytes",
            self.frames, self.keyframes, self.max_chunk
        );
        println!(
            "palettes    {} records, largest component {:#04x}",
            self.palette_records, self.max_component
        );
        println!(
            "audio       {} files, {} sub-chunks, {} bytes decoded",
            self.audio_files, self.audio_chunks, self.audio_bytes
        );
        println!("dimensions  {dims}");
        println!("frame times {times} (10µs units)");
        if !configs.is_empty() {
            println!("audio       {configs}");
        }
    }
}

fn join(items: impl Iterator<Item = String>) -> String {
    items.collect::<Vec<_>>().join(", ")
}

fn layout(channels: u8) -> &'static str {
    if channels == 2 { "stereo" } else { "mono" }
}

/// Tests for the harness itself: a gate that cannot fail is not a gate.
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn padding_overrun_spots_a_disturbed_column() {
        let (pitch, width, height) = (6, 4, 3);
        let mut dst = vec![PAD_FILL; pitch * height];
        assert_eq!(padding_overrun(&dst, pitch, width, height), None);
        dst[pitch + width + 1] = 0;
        assert_eq!(padding_overrun(&dst, pitch, width, height), Some(1));
    }

    #[test]
    fn findings_are_capped_but_counted() {
        let mut findings = Findings::default();
        assert!(findings.is_empty());
        for i in 0..MAX_REPORTED + 3 {
            findings.push(format!("finding {i}"));
        }
        assert_eq!(findings.items.len(), MAX_REPORTED);
        assert_eq!(findings.total, MAX_REPORTED + 3);
    }

    /// Truncated input must be reported, never panic. Skips when the corpus
    /// is unavailable.
    #[test]
    fn truncated_input_is_reported() {
        let mut files = Vec::new();
        if collect(&default_corpus(), &mut files).is_err() || files.is_empty() {
            eprintln!("corpus unavailable; skipping");
            return;
        }
        files.sort();
        for path in files.iter().take(8) {
            let Ok(bytes) = std::fs::read(path) else {
                continue;
            };
            let (_, findings) = check_bytes(&bytes);
            assert!(
                findings.is_empty(),
                "{}: {:?}",
                path.display(),
                findings.items
            );
            for n in 1..=16 {
                let cut = bytes.len() * n / 20;
                let (_, findings) = check_bytes(&bytes[..cut]);
                assert!(
                    !findings.is_empty(),
                    "{}: truncation to {cut} bytes went unreported",
                    path.display()
                );
            }
        }
    }
}
