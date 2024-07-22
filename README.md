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

## Running
__ReMech 2 does not include any part of the original game data.__

Documentation is forthcoming. You'll need an installed copy of the orginal
Windows 95 version (a.k.a. Pentium Edition), a specific version of the game's
DLL files contained in the
[Windows 95 1.1 _patch_](https://archive.org/details/mw2patch), not the 1.1 CD.

You'll also need the game's CD inserted in order to play, but it can be any
version of the original software-rendered release, DOS or Windows. The game's
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