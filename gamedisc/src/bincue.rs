use std::{
    fs,
    io::{self, BufReader, Read, Seek, SeekFrom},
    path::Path,
    sync::{Mutex, PoisonError},
};

use crate::{
    Error, SectorSource,
    cue::{CueFileKind, CueSheet, Layout, resolve_file},
};

struct BinFile {
    reader: Mutex<BufReader<fs::File>>,
    sector_size: u32,
}

pub struct BinCueSource {
    layout: Layout,
    files: Vec<BinFile>,
}

impl BinCueSource {
    pub fn open<P: AsRef<Path>>(cue: P) -> Result<Self, Error> {
        let cue = cue.as_ref();
        let cue_sheet = CueSheet::parse(&read_cue(cue)?)?;
        let cue_dir = cue.parent().unwrap_or(Path::new(""));

        let mut file_sectors = vec![];
        let mut files = vec![];

        for file in &cue_sheet.files {
            let path = resolve_file(cue_dir, &file.path)?;

            if file.kind != CueFileKind::Binary {
                return Err(Error::NotBinary(path));
            }

            let sector_size = file
                .tracks
                .first()
                .expect("file to have at least one track")
                .mode
                .sector_size();

            let f = fs::File::open(&path)?;
            let filesize = f.metadata()?.len();

            if filesize % u64::from(sector_size) != 0 {
                return Err(Error::PartialSector(path));
            }

            let sectors = u32::try_from(filesize / u64::from(sector_size))
                .map_err(|_| io::Error::from(io::ErrorKind::FileTooLarge))?;

            file_sectors.push(sectors);
            files.push(BinFile {
                reader: Mutex::new(BufReader::new(f)),
                sector_size,
            });
        }

        Ok(Self {
            layout: cue_sheet.layout(&file_sectors)?,
            files,
        })
    }
}

fn read_cue(path: &Path) -> io::Result<String> {
    let bytes = fs::read(path)?;
    Ok(String::from_utf8(bytes)
        .unwrap_or_else(|e| e.into_bytes().into_iter().map(char::from).collect()))
}

fn bcd_msf(lba: u32) -> [u8; 3] {
    let frames = lba + 150;
    let bcd = |n: u32| (((n / 10) << 4) | (n % 10)) as u8;
    [bcd(frames / 4500), bcd(frames / 75 % 60), bcd(frames % 75)]
}

impl SectorSource for BinCueSource {
    fn tracks(&self) -> &[crate::Track] {
        &self.layout.tracks
    }

    fn read_raw(&self, lba: u32, count: u32, out: &mut [u8]) -> Result<(), Error> {
        const RAW_SECTOR: usize = 2352;
        const SYNC: [u8; 12] = [
            0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
        ];

        let end = lba
            .checked_add(count)
            .filter(|&end| end <= self.layout.leadout_lba)
            .ok_or(Error::OutOfRange { lba, count })?;
        assert_eq!(
            out.len(),
            count as usize * RAW_SECTOR,
            "`out` must hold `count` raw sectors"
        );

        for (span, file) in self.layout.files.iter().zip(&self.files) {
            let first = lba.max(span.start_lba);
            let last = end.min(span.start_lba + span.sectors);
            if first >= last {
                continue;
            }

            let run =
                &mut out[(first - lba) as usize * RAW_SECTOR..(last - lba) as usize * RAW_SECTOR];
            let mut reader = file.reader.lock().unwrap_or_else(PoisonError::into_inner);
            reader.seek(SeekFrom::Start(
                u64::from(first - span.start_lba) * u64::from(file.sector_size),
            ))?;

            if file.sector_size as usize == RAW_SECTOR {
                reader.read_exact(run)?;
            } else {
                let (chunks, _) = run.as_chunks_mut::<RAW_SECTOR>();
                for (sector, lba) in chunks.iter_mut().zip(first..) {
                    let (header, rest) = sector.split_at_mut(16);
                    let (user, ecc) = rest.split_at_mut(2048);
                    header[..12].copy_from_slice(&SYNC);
                    header[12..15].copy_from_slice(&bcd_msf(lba));
                    header[15] = 1;
                    reader.read_exact(user)?;
                    ecc.fill(0);
                }
            }
        }

        Ok(())
    }
}
