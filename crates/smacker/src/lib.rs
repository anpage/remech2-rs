//! Smacker (`SMK2`) FMV decoder for `ReMech 2`.
//!
//! Scoped to what the shipped `ReMech 2` corpus actually needs: SMK2 only, no
//! keyframes, one palette record, track-0 DPCM audio.
//!
//! Extension seams (SMK4, ring frames, Y-scaling, multi-track, uncompressed
//! audio) are deliberately left open; none of them affect the corpus.
//!
//! # Disclaimer
//!
//! Except for this disclaimer, this crate is entirely written by AI
//! from spec documents and SMACKW32.DLL reversing. Its output is verified
//! byte-identical to the DLL for every FMV that shipped with the original game.
//!
//! Everything is documented (albeit in overly verbose LLM-speak), no unsafe
//! code is allowed, and it fully passes clippy and all of the tests. I also
//! visually verified that all of the FMVs and Smacker-animated UI elements
//! render correctly in the shell.
//!
//! It may be a bit of a black box, but it's an improvement over requiring a
//! proprietary binary to decode the game's FMVs.

#![forbid(unsafe_code)]
#![warn(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

mod audio;
mod bitreader;
mod decoder;
mod error;
mod header;
mod huffman;
mod palette;
mod video;

pub use audio::{AudioDecoder, DecodedAudio};
pub use bitreader::BitReader;
pub use decoder::Decoder;
pub use error::Error;
pub use header::{
    frame_time_from_rate, AudioInfo, FrameTime10us, Header, VideoInfo, FRAME_TYPE_PALETTE,
    FRAME_TYPE_TRACK_BASE, TRACK_COUNT,
};
pub use huffman::{HeaderTree, HuffmanTables};
pub use palette::{split_record, Palette, PALETTE_ENTRIES};
pub use video::VideoDecoder;

/// Corpus-driven validation over the shipped `.smk` files.
///
/// These tests need the game data; they skip silently when `data/SMK` (or
/// the directory named by the `SMACKER_CORPUS` env var) is unavailable.
#[cfg(all(test, feature = "std"))]
mod corpus_tests {
    use super::*;
    use std::path::PathBuf;

    fn corpus_files() -> Vec<PathBuf> {
        let dir = std::env::var_os("SMACKER_CORPUS").map_or_else(
            || PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../data/SMK"),
            PathBuf::from,
        );
        if !dir.is_dir() {
            eprintln!("corpus dir {} not found; skipping", dir.display());
            return Vec::new();
        }
        let mut files: Vec<PathBuf> = std::fs::read_dir(&dir)
            .unwrap()
            .filter_map(Result::ok)
            .map(|e| e.path())
            .filter(|p| p.extension().is_some_and(|e| e.eq_ignore_ascii_case("smk")))
            .collect();
        files.sort();
        files
    }

    #[test]
    fn corpus_parses_with_expected_invariants() {
        let files = corpus_files();
        if files.is_empty() {
            return;
        }
        for path in &files {
            let bytes = std::fs::read(path).unwrap();
            let hdr = Header::parse(&bytes).unwrap_or_else(|e| panic!("{}: {e}", path.display()));

            // SMK2, flags == 0 (checked by parse), dimensions % 4.
            let v = hdr.video_info();
            assert_eq!(v.width % 4, 0, "{}", path.display());
            assert_eq!(v.height % 4, 0, "{}", path.display());

            // Frame rate field is always negative in the corpus.
            assert!(v.frame_time.0 > 0, "{}", path.display());

            // Exactly one palette record, on frame 0; no keyframes.
            assert_ne!(
                hdr.frame_type(0) & FRAME_TYPE_PALETTE,
                0,
                "{}: frame 0 has no palette record",
                path.display()
            );
            for i in 0..hdr.table_entries() {
                assert_eq!(
                    hdr.frame_type(i) & !0x03,
                    0,
                    "{}: frame {i} has unexpected type bits",
                    path.display()
                );
                assert!(
                    !hdr.is_keyframe(i),
                    "{}: frame {i} is a keyframe",
                    path.display()
                );
                if i > 0 {
                    assert_eq!(
                        hdr.frame_type(i) & FRAME_TYPE_PALETTE,
                        0,
                        "{}: frame {i} has a palette record",
                        path.display()
                    );
                }
                // Every frame chunk must lie fully inside the file.
                hdr.frame_chunk(&bytes, i)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
            }

            // Audio: track 0 only, always compressed, known configs.
            for t in 0..TRACK_COUNT {
                match hdr.audio_track(t) {
                    Some(a) => {
                        assert_eq!(t, 0, "{}: audio on track {t}", path.display());
                        assert!(a.compressed, "{}", path.display());
                        assert!(
                            matches!(
                                (a.bits, a.channels, a.rate),
                                (8, 1, 22050 | 11025) | (16, 2, 11025)
                            ),
                            "{}: unexpected audio config {a:?}",
                            path.display()
                        );
                    }
                    None => assert_eq!(
                        hdr.frame_type(0) & (FRAME_TYPE_TRACK_BASE << t),
                        0,
                        "{}: frame 0 sets track {t} bit but header has no track",
                        path.display()
                    ),
                }
            }
        }
        eprintln!("parsed {} corpus files OK", files.len());
    }

    #[test]
    fn corpus_huffman_trees_parse() {
        let files = corpus_files();
        if files.is_empty() {
            return;
        }
        for path in &files {
            let bytes = std::fs::read(path).unwrap();
            let hdr = Header::parse(&bytes).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            HuffmanTables::parse(hdr.tree_data(), hdr.tree_sizes())
                .unwrap_or_else(|e| panic!("{}: tree parse: {e}", path.display()));
        }
        eprintln!("parsed Huffman trees of {} corpus files OK", files.len());
    }

    #[test]
    fn corpus_palette_records_decode() {
        let files = corpus_files();
        if files.is_empty() {
            return;
        }
        for path in &files {
            let bytes = std::fs::read(path).unwrap();
            let hdr = Header::parse(&bytes).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            let mut palette = Palette::new();
            let mut records = 0;
            for i in 0..hdr.table_entries() {
                if hdr.frame_type(i) & FRAME_TYPE_PALETTE == 0 {
                    continue;
                }
                records += 1;
                let chunk = hdr
                    .frame_chunk(&bytes, i)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
                let (ops, _) = split_record(chunk)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
                palette
                    .apply_record(ops)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
            }
            assert_eq!(records, 1, "{}", path.display());
            for (i, entry) in palette.current().iter().enumerate() {
                assert!(
                    entry.iter().all(|&c| c <= 0x3F),
                    "{}: palette entry {i} has a component > 0x3F: {entry:?}",
                    path.display()
                );
            }
        }
        eprintln!("decoded palette records of {} corpus files OK", files.len());
    }

    #[test]
    fn corpus_video_frames_decode() {
        let files = corpus_files();
        if files.is_empty() {
            return;
        }
        let mut total_frames = 0usize;
        for path in &files {
            let bytes = std::fs::read(path).unwrap();
            let hdr = Header::parse(&bytes).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            let tables = HuffmanTables::parse(hdr.tree_data(), hdr.tree_sizes())
                .unwrap_or_else(|e| panic!("{}: tree parse: {e}", path.display()));
            let info = hdr.video_info();
            let mut dec = VideoDecoder::new(&info, tables);
            let mut palette = Palette::new();

            // Exercise a non-trivial pitch and both flip orientations.
            let pitch = info.width as usize + 8;
            let mut dst = vec![0u8; pitch * info.height as usize];

            for i in 0..hdr.table_entries() {
                let chunk = hdr
                    .frame_chunk(&bytes, i)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
                if hdr.frame_type(i) & FRAME_TYPE_PALETTE != 0 {
                    let (ops, _) = split_record(chunk)
                        .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
                    palette
                        .apply_record(ops)
                        .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
                }
                let bits = hdr
                    .video_subchunk(chunk, i)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
                let consumed = dec
                    .decode_frame(bits, &mut dst, pitch, i % 2 == 0)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
                // The video bitstream ends 0–4 bytes short of the chunk end
                // (dword padding).
                assert!(
                    bits.len() - consumed <= 4,
                    "{}: frame {i}: {} unconsumed video bytes",
                    path.display(),
                    bits.len() - consumed
                );
                total_frames += 1;
            }
        }
        eprintln!(
            "decoded {total_frames} video frames across {} corpus files OK",
            files.len()
        );
    }

    /// The stream-level API driven the way the shell drives it: open, then
    /// per frame decode into the destination and step forward.
    #[test]
    fn corpus_decodes_through_the_stream_api() {
        let files = corpus_files();
        if files.is_empty() {
            return;
        }
        for path in &files {
            let bytes = std::fs::read(path).unwrap();
            let mut dec =
                Decoder::open(&bytes).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            let info = dec.video_info();

            // A non-trivial pitch, as the game uses (640-wide back buffer).
            let pitch = info.width as usize + 8;
            let mut dst = vec![0u8; pitch * info.height as usize];

            assert!(
                dec.palette_changed(),
                "{}: frame 0 did not report a new palette",
                path.display()
            );

            for i in 0..info.frames {
                assert_eq!(dec.frame_index(), i, "{}", path.display());
                dec.decode_frame(&mut dst, pitch, false)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
                assert!(
                    dec.palette().iter().flatten().all(|&c| c <= 0x3F),
                    "{}: frame {i}: palette component > 0x3F",
                    path.display()
                );
                if i + 1 < info.frames {
                    assert!(
                        dec.next_frame()
                            .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display())),
                        "{}: frame {i}: stream wrapped early",
                        path.display()
                    );
                }
            }

            // Looping back to the start must give the same first frame as a
            // fresh open, since the corpus has no keyframes.
            dec.seek(0)
                .unwrap_or_else(|e| panic!("{}: seek: {e}", path.display()));
            dec.decode_frame(&mut dst, pitch, false)
                .unwrap_or_else(|e| panic!("{}: replayed frame 0: {e}", path.display()));
            let fresh = Decoder::open(&bytes).unwrap();
            let mut first = vec![0u8; pitch * info.height as usize];
            let mut fresh = fresh;
            fresh.decode_frame(&mut first, pitch, false).unwrap();
            assert_eq!(dst, first, "{}: replayed frame 0 differs", path.display());
        }
        eprintln!("stream-decoded {} corpus files OK", files.len());
    }

    /// FNV-1a (64-bit): a stable hash that does not depend on the toolchain,
    /// so the golden table below stays valid across compiler versions.
    fn fnv1a(data: &[u8]) -> u64 {
        let mut hash = 0xcbf2_9ce4_8422_2325u64;
        for &b in data {
            hash ^= u64::from(b);
            hash = hash.wrapping_mul(0x0000_0100_0000_01b3);
        }
        hash
    }

    /// Golden-output spot checks: pixel placement is not covered by the
    /// bitstream-consumption checks, so hash decoded frames of a few
    /// representative files — the smallest, the largest, a 16-bit-stereo
    /// one, and `MINTRO.SMK` — and compare against known-good values.
    /// Frame 0 and the mid-file frame of each are checked, plus frame 0's
    /// palette.
    #[test]
    fn corpus_golden_frames() {
        struct Frame {
            pixel: u32,
            hash: u64,
        }

        struct Golden {
            file: &'static str,
            frames: &'static [Frame],
            /// frame-0 palette hash
            palette: u64,
        }

        const GOLDEN: &[Golden] = &[
            // 16-bit-stereo audio.
            Golden {
                file: "MEND.SMK",
                frames: &[
                    Frame {
                        pixel: 0,
                        hash: 0xe4a97b12f0fd5361,
                    },
                    Frame {
                        pixel: 203,
                        hash: 0xeed1f39950419b6e,
                    },
                ],
                palette: 0xb51dd93e8c75c692,
            },
            // The largest file.
            Golden {
                file: "MINTRO.SMK",
                frames: &[
                    Frame {
                        pixel: 0,
                        hash: 0x3c0bf964b70ad325,
                    },
                    Frame {
                        pixel: 581,
                        hash: 0x735b0779a98e65d6,
                    },
                ],
                palette: 0x8cda43b27df13b2d,
            },
            Golden {
                file: "MWOLAND.SMK",
                frames: &[
                    Frame {
                        pixel: 0,
                        hash: 0x846384e1f421c475,
                    },
                    Frame {
                        pixel: 364,
                        hash: 0xcdd31257b59bf6ab,
                    },
                ],
                palette: 0x2e4bf57e4c4254bd,
            },
            // The smallest file (a single frame).
            Golden {
                file: "WWOCN.SMK",
                frames: &[Frame {
                    pixel: 0,
                    hash: 0x971a8bf69a332afc,
                }],
                palette: 0x709442c7d570e4b1,
            },
        ];

        let files = corpus_files();
        if files.is_empty() {
            return;
        }
        let update = std::env::var_os("SMACKER_GOLDEN_UPDATE").is_some();
        for Golden {
            file: name,
            frames,
            palette: palette_hash,
        } in GOLDEN
        {
            let path = files
                .iter()
                .find(|p| p.file_name().is_some_and(|f| f == *name))
                .unwrap_or_else(|| panic!("{name}: not in the corpus"));
            let bytes = std::fs::read(path).unwrap();
            let mut dec = Decoder::open(&bytes).unwrap_or_else(|e| panic!("{name}: {e}"));
            let info = dec.video_info();
            let mut dst = vec![0u8; (info.width * info.height) as usize];

            let mut palette_checked = false;
            for Frame {
                pixel: target,
                hash: expected,
            } in *frames
            {
                assert!(*target < info.frames, "{name}: no frame {target}");
                dec.seek(*target)
                    .unwrap_or_else(|e| panic!("{name}: seek to {target}: {e}"));
                dec.decode_frame(&mut dst, info.width as usize, false)
                    .unwrap_or_else(|e| panic!("{name}: frame {target}: {e}"));
                let actual = fnv1a(dec.frame());
                if update {
                    eprintln!("{name} frame {target}: {actual:#018x}");
                } else {
                    assert_eq!(
                        actual, *expected,
                        "{name}: frame {target} pixel hash changed (got {actual:#018x})"
                    );
                }
                if *target == 0 && !palette_checked {
                    palette_checked = true;
                    let actual = fnv1a(dec.palette().as_flattened());
                    if update {
                        eprintln!("{name} palette: {actual:#018x}");
                    } else {
                        assert_eq!(
                            actual, *palette_hash,
                            "{name}: frame 0 palette hash changed (got {actual:#018x})"
                        );
                    }
                }
            }
        }
    }

    /// Whole-track audio extraction, as the shell needs it for playback.
    #[test]
    fn corpus_tracks_decode_whole() {
        let files = corpus_files();
        if files.is_empty() {
            return;
        }
        let mut with_audio = 0usize;
        for path in &files {
            let bytes = std::fs::read(path).unwrap();
            let dec = Decoder::open(&bytes).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            let Some(info) = dec.audio_track(0) else {
                for t in 1..TRACK_COUNT {
                    assert!(dec.decode_track(t).unwrap().is_none(), "{}", path.display());
                }
                continue;
            };
            with_audio += 1;
            let pcm = dec
                .decode_track(0)
                .unwrap_or_else(|e| panic!("{}: {e}", path.display()))
                .expect("track 0 exists");
            let frame_bytes = usize::from(info.channels) * usize::from(info.bits / 8);
            assert!(!pcm.is_empty(), "{}", path.display());
            assert_eq!(pcm.len() % frame_bytes, 0, "{}", path.display());
        }
        eprintln!("decoded whole audio tracks of {with_audio} corpus files OK");
    }

    /// Not-present packets contribute silence of the declared length. For
    /// 8-bit (unsigned) tracks that silence is the 0x80 midpoint, so the
    /// decoded track must never contain a long run of 0x00 — that would be
    /// a full-scale DC step, audible as a pop.
    #[test]
    fn corpus_8bit_tracks_have_no_zero_silence_runs() {
        let files = corpus_files();
        if files.is_empty() {
            return;
        }
        for path in &files {
            let bytes = std::fs::read(path).unwrap();
            let dec = Decoder::open(&bytes).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            let Some(info) = dec.audio_track(0) else {
                continue;
            };
            if info.bits != 8 {
                continue;
            }
            let pcm = dec
                .decode_track(0)
                .unwrap_or_else(|e| panic!("{}: {e}", path.display()))
                .expect("track 0 exists");
            let run = pcm
                .iter()
                .fold((0usize, 0usize), |(cur, max), &b| {
                    let cur = if b == 0 { cur + 1 } else { 0 };
                    (cur, max.max(cur))
                })
                .1;
            assert!(
                run < 64,
                "{}: {run} consecutive zero bytes in an 8-bit track \
                 (not-present packet filled with 0x00 instead of 0x80?)",
                path.display()
            );
        }
    }

    #[test]
    fn corpus_audio_subchunks_decode() {
        let files = corpus_files();
        if files.is_empty() {
            return;
        }
        let mut total_chunks = 0usize;
        let mut files_with_audio = 0usize;
        for path in &files {
            let bytes = std::fs::read(path).unwrap();
            let hdr = Header::parse(&bytes).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            let Some(info) = hdr.audio_track(0) else {
                continue;
            };
            files_with_audio += 1;
            let mut dec =
                AudioDecoder::new(info).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            let mut dst = vec![0u8; info.max_unpacked_size as usize];

            for i in 0..hdr.table_entries() {
                let chunk = hdr
                    .frame_chunk(&bytes, i)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
                let Some(sub) = hdr
                    .audio_subchunk(chunk, i, 0)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()))
                else {
                    continue;
                };
                let r = dec
                    .decode(sub, &mut dst)
                    .unwrap_or_else(|e| panic!("{}: frame {i}: {e}", path.display()));
                // The bitstream ends 0–4 bytes short of the sub-chunk end
                // (dword padding).
                assert!(
                    sub.len() - r.bytes_consumed <= 4,
                    "{}: frame {i}: {} unconsumed audio bytes",
                    path.display(),
                    sub.len() - r.bytes_consumed
                );
                total_chunks += 1;
            }
        }
        eprintln!(
            "decoded {total_chunks} audio sub-chunks across {files_with_audio} corpus files OK"
        );
    }
}
