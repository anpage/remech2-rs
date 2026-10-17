use std::io;

use hadris_iso::{
    IsoImage,
    file::EntryType,
    read::{DirEntry, RootDir},
};

use crate::{Error, RAW_SECTOR, SYNC, SectorSource, TrackKind};

const BLOCK: u64 = 2048;

/// The 2048-byte user data of a disc's data track
pub struct LogicalBlocks<S> {
    source: S,
    /// First LBA of the data track
    start_lba: u32,
    /// Length of the data track in sectors
    sectors: u32,
    /// 16 for mode 1, 24 for mode 2 form 1
    data_offset: usize,
    /// Position in the user data, 2048 bytes per sector
    pos: u64,
}

impl<S: SectorSource> LogicalBlocks<S> {
    pub fn new(source: S) -> Result<Self, Error> {
        let (start_lba, sectors, kind) = {
            let track = source
                .tracks()
                .iter()
                .find(|t| t.kind != TrackKind::Audio && t.sectors > 0)
                .ok_or(Error::NoDataTrack)?;
            (track.start_lba, track.sectors, track.kind)
        };

        let mut first = [0; RAW_SECTOR];
        source.read_raw(start_lba, 1, &mut first)?;

        let data_offset = match (first[..12] == SYNC, first[15], kind) {
            (true, 1, _) | (false, _, TrackKind::Mode1) => 16,
            (true, 2, _) | (false, _, TrackKind::Mode2Form1) => 24,
            (true, _, TrackKind::Mode2Form1) => 24,
            _ => 16,
        };

        Ok(Self {
            source,
            start_lba,
            sectors,
            data_offset,
            pos: 0,
        })
    }
}

impl<S: SectorSource> io::Read for LogicalBlocks<S> {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        let end = u64::from(self.sectors) * BLOCK;
        if self.pos >= end || buf.is_empty() {
            return Ok(0);
        }

        let sector = (self.pos / BLOCK) as u32;
        let within = (self.pos % BLOCK) as usize;
        let n = buf.len().min(BLOCK as usize - within);

        let mut raw = [0; RAW_SECTOR];
        self.source
            .read_raw(self.start_lba + sector, 1, &mut raw)
            .map_err(io::Error::other)?;

        let start = self.data_offset + within;
        buf[..n].copy_from_slice(&raw[start..start + n]);
        self.pos += n as u64;

        Ok(n)
    }
}

impl<S: SectorSource> io::Seek for LogicalBlocks<S> {
    fn seek(&mut self, pos: io::SeekFrom) -> io::Result<u64> {
        let end = u64::from(self.sectors) * BLOCK;

        let destination = match pos {
            io::SeekFrom::Start(p) => Some(p),
            io::SeekFrom::End(p) => end.checked_add_signed(p),
            io::SeekFrom::Current(p) => self.pos.checked_add_signed(p),
        };

        self.pos = destination.ok_or(io::ErrorKind::InvalidInput)?;

        Ok(self.pos)
    }
}

/// The ISO 9660 tree of an image
pub struct IsoFs<D: io::Seek> {
    image: IsoImage<D>,
    root: RootDir,
}

impl<D: io::Read + io::Seek> IsoFs<D> {
    pub fn open(data: D) -> Result<Self, Error> {
        let image = IsoImage::open(data)?;

        let root = image
            .root_dirs()
            .iter()
            .find(|root| !matches!(root.entry_type(), EntryType::Joliet { .. }))
            .copied()
            .ok_or(Error::NoDirectoryTree)?;

        Ok(Self { image, root })
    }

    pub fn has_file(&self, path: &str) -> Result<bool, Error> {
        Ok(self.lookup(path)?.is_some())
    }

    /// Streams the file into `out`.
    /// Returns its size or `None` if there is no such file.
    pub fn extract(&self, path: &str, out: &mut impl io::Write) -> Result<Option<u64>, Error> {
        let Some(entry) = self.lookup(path)? else {
            return Ok(None);
        };

        let mut written = 0;
        let mut chunks = self.image.read_file_chunked::<65536>(&entry)?;
        while let Some(chunk) = chunks.next_chunk()? {
            out.write_all(&chunk)?;
            written += chunk.len() as u64;
        }

        Ok(Some(written))
    }

    fn lookup(&self, path: &str) -> Result<Option<DirEntry>, Error> {
        let mut dir = self.root.iter(&self.image);
        let mut parts = path.split(['/', '\\']).filter(|p| !p.is_empty()).peekable();

        while let Some(part) = parts.next() {
            let Some(entry) = dir.find(part)? else {
                return Ok(None);
            };

            if parts.peek().is_none() {
                return Ok(entry.is_file().then_some(entry));
            }

            if !entry.is_directory() {
                return Ok(None);
            }

            dir = self.image.open_dir(entry.as_dir_ref(&self.image)?);
        }

        Ok(None)
    }
}
