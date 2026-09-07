# ReMech 2

ReMech 2 is an **unofficial** open-source replacement executable for the Windows
95 version of the game _MechWarrior 2: 31st Century Combat_, built for modern
Windows.

The goal is to reimplement the entire engine function-by-function, focusing on
fixing bugs and adjusting for modern versions of Windows on the way.

In the interest of getting the game working on modern machines, there are
temporarily some shims for Win32 libraries that will be removed once enough of
the game's code can be replaced to no longer need them.

This is still a work-in-progress and things are very rough and hacky, but the
game currently works well enough to play on Windows 11.

## Features and Fixes

The fixes from Chris Porter's [Windows XP patch](https://www.warp13.co.uk/mech2) are reimplemented:

- Fixes crash when launching the shell
- Fixes Mech Lab crash
- Fixes many heap-related crashes during gameplay
- Limits the framerate to 45 FPS to fix problems with physics and jump jet fuel recharging

Additionally:

- Adds a "launcher" that runs before the sim
  - Checks the game's files
  - Can install the game from CD
  - Can download and install the required v1.1 patch
- Fixes an issue where it was sometimes impossible to explode from overheating
- Fixes stuttering when using a mouse with a high poll rate
- Fixes the background music restarting when the game is paused
- Fixes an error in Windows 11 that broke CD audio playback
- Plays back music from files if present
- Fixes freezing related to flawed multithreading
- Fixes problems accessing registry keys without running as admin
- Fixes an issue where the intro video could cause a freeze
- Restores the 1024x768 in-game resolution option from the DOS version
- Restores the custom cursor image from the DOS version
- Replaces MIDI playback with an internal synthesizer
- Replaces Miles Sound System (WAIL32.DLL) with a modern library
- Allows arbitrary window sizes and upscales the game with the correct aspect ratio
- Replaces the Windows menu bar with one that's rendered on top of the shell

There is more to come as reimplementation progresses.

## Running

**ReMech 2 does not include any part of the original game data.**

Documentation is forthcoming. You'll need an installed copy of the orginal
Windows 95 version (a.k.a. Pentium Edition) and a specific version of the game's
DLL files contained in the
[Windows 95 1.1 _patch_](https://archive.org/details/mw2patch), not the 1.1 CD.

If you don't currently have the game installed, run Remech 2 from within its own
(writable) folder with the CD inserted. It can pull all the necessary files
from the CD and install the 1.1 patch from the internet automatically.

### Music

ReMech 2 can play the game's background music from files instead of from the CD.
Put them in a `Music` folder in the game's working directory, named `track02.wav`
through `track99.wav`. The numbering matches the CD's original track numbers,
which is why it starts at 2. Track 1 on the disc is always game data. OGG and
MP3 files work as well and file names are matched case-insensitively.

If the folder is missing, or contains no files matching that pattern, the game
falls back to playing music from the CD.

Both defaults can be overridden in the `remech2.ini` that is created in the
working directory the first time the game is run.

```toml
[audio]
music_path="Music"
cd_source="auto"
```

`music_path` can either be relative to the working directory or an absolute
path.

`cd_source` can be either `"auto"` (default), `"files"`, or `"mci"`. `"files"`
will play music from files at `music_path`, `"auto"` will fall back to playing
music from CD if `music_path` is not present or has no usable tracks, and
`"mci"` will only play music from CD and ignore `music_path`.

## Building

### Requirements

- [The Rust toolchain](https://rustup.rs/)
- **Nightly**, Windows, MSVC

### Steps

Nothing special for a Rust project. Just:

`cargo build`

Until the dependency on the original game's DLLs is lifted, a 32-bit build
target is required.

## License

The source code provided in this repository is licensed under the
[MIT License](LICENSE.md).

ReMech dynamically links with the proprietary code within the original game's
DLL files in order to fill in the gaps until everything is 100% reimplemented.

ReMech2 is in no way associated with or endorsed by Activision Blizzard, Inc. or
any other company.

GeneralUser GS by S. Christian Collins is included as the default soundfont.
See `GUGS-LICENSE.txt` for more information.
