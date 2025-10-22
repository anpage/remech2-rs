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
- Fixes freezing related to flawed multithreading
- Fixes problems accessing registry keys without running as admin
- Fixes an issue where the intro video could cause a freeze
- Restores the 1024x768 in-game resolution option from the DOS version
- Restores the custom cursor image from the DOS version
- Replaces MIDI playback with an internal synthesizer
- Replaces the audio subsystem of the shell with a modern library
- Allows arbitrary window sizes and upscales the game with the correct aspect ratio
- Replaces the Windows menu bar with one that's rendered on top of the shell

There is more to come as reimplementation progresses.

## Running
__ReMech 2 does not include any part of the original game data.__

Documentation is forthcoming. You'll need an installed copy of the orginal
Windows 95 version (a.k.a. Pentium Edition) and a specific version of the game's
DLL files contained in the
[Windows 95 1.1 _patch_](https://archive.org/details/mw2patch), not the 1.1 CD.

If you don't currently have the game installed, run Remech 2 from within its own
(writable) folder with the CD mounted. It can pull all the necessary files
from the CD and install the 1.1 patch from the internet automatically.

You'll also need the game's CD inserted in order to play, but it can be any
copy of the original Windows 95 software-rendered release. The game's
copy-protection has not been removed.

## Building
### Requirements
* [The Rust toolchain](https://rustup.rs/)
* **Nightly**, Windows, MSVC

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