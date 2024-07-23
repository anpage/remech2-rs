#![feature(layout_for_ptr)]
#![feature(c_variadic)]

use anyhow::{bail, Result};
use std::{
    env,
    fs::File,
    io::{BufReader, Read, Seek, SeekFrom},
};
use windows::{
    core::{s, PCSTR},
    Win32::{
        Foundation::*,
        Graphics::Gdi::*,
        Storage::FileSystem::{GetDriveTypeA, GetLogicalDriveStringsA},
        System::{LibraryLoader::GetModuleHandleA, WindowsProgramming::DRIVE_CDROM},
        UI::WindowsAndMessaging::*,
    },
};

mod ail;
mod common;
mod hooker;
mod midi_source;
mod shell;
mod sim;
mod xmi;

type WindowProc = unsafe extern "system" fn(HWND, u32, WPARAM, LPARAM) -> LRESULT;

static mut SIM_WINDOW_PROC: Option<WindowProc> = None;
static mut SHELL_WINDOW_PROC: Option<WindowProc> = None;

enum ProcessType {
    None,
    Sim,
    Shell,
}

static mut PROCESS_TYPE: ProcessType = ProcessType::None;

extern "system" fn wnd_proc(window: HWND, message: u32, wparam: WPARAM, lparam: LPARAM) -> LRESULT {
    unsafe {
        match message {
            0x41E => {
                PROCESS_TYPE = ProcessType::None;
            }
            0x41F => {
                PROCESS_TYPE = ProcessType::Sim;
            }
            0x420 => {
                PROCESS_TYPE = ProcessType::Shell;
            }
            _ => {}
        }

        match PROCESS_TYPE {
            ProcessType::None => {}
            ProcessType::Sim => {
                if let Some(proc) = SIM_WINDOW_PROC {
                    return proc(window, message, wparam, lparam);
                }
            }
            ProcessType::Shell => {
                if let Some(proc) = SHELL_WINDOW_PROC {
                    return proc(window, message, wparam, lparam);
                }
            }
        }

        DefWindowProcA(window, message, wparam, lparam)
    }
}

fn create_window(instance: HINSTANCE, width: i32, height: i32) -> Result<HWND> {
    unsafe {
        let class_name = s!("REMECH 2");

        let wc = WNDCLASSA {
            style: CS_HREDRAW | CS_VREDRAW,
            lpfnWndProc: Some(wnd_proc),
            cbClsExtra: 0,
            cbWndExtra: 0,
            hInstance: instance,
            hbrBackground: HBRUSH(GetStockObject(BLACK_BRUSH).0),
            lpszMenuName: PCSTR::null(),
            lpszClassName: class_name,
            ..Default::default()
        };

        let atom = RegisterClassA(&wc);
        debug_assert!(atom != 0);

        let mut window_rect = RECT {
            left: 0,
            top: 0,
            right: width,
            bottom: height,
        };

        let style = {
            let display_width = GetSystemMetrics(SM_CXSCREEN);
            let display_height = GetSystemMetrics(SM_CYSCREEN);
            if width < display_width || height < display_height {
                let style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
                AdjustWindowRect(&mut window_rect, style, false)?;
                window_rect.right -= window_rect.left;
                window_rect.bottom -= window_rect.top;
                window_rect.top = (display_height - window_rect.bottom) / 2;
                window_rect.left = (display_width - window_rect.right) / 2;
                style
            } else {
                WS_POPUP
            }
        };

        let window = CreateWindowExA(
            WS_EX_LEFT,
            class_name,
            s!("REMECH 2"),
            style,
            window_rect.left,
            window_rect.top,
            window_rect.right,
            window_rect.bottom,
            None,
            None,
            instance,
            None,
        )?;

        SetMenu(window, None)?;
        let _ = ShowWindow(window, SW_SHOWDEFAULT);
        let _ = UpdateWindow(window);

        Ok(window)
    }
}

fn start_shell(window: HWND, intro_or_sim: &str) -> Result<i32> {
    let shell = shell::Shell::new()?;
    unsafe { SHELL_WINDOW_PROC = Some(shell.window_proc()?) };
    let result = shell.shell_main(intro_or_sim, window)?;
    unsafe { SHELL_WINDOW_PROC = None };
    Ok(result)
}

fn start_sim(window: HWND, cmd_line: &str) -> Result<i32> {
    let sim = sim::Sim::new()?;
    unsafe { SIM_WINDOW_PROC = Some(sim.window_proc()?) };
    let result = sim.sim_main(cmd_line, std::ptr::null(), FALSE, window)?;
    unsafe { SIM_WINDOW_PROC = None };
    Ok(result)
}

fn cd_check() -> bool {
    let mut drive_strings = [0u8; 128];
    unsafe {
        GetLogicalDriveStringsA(Some(&mut drive_strings));
    }

    for drive in drive_strings.split(|&c| c == 0) {
        if drive.is_empty() {
            continue;
        }

        let drive_type = unsafe { GetDriveTypeA(PCSTR(drive.as_ptr())) };

        if drive_type != DRIVE_CDROM {
            continue;
        }

        let path = format!("{}:\\OLD_HERC.DRV", *drive.first().unwrap() as char);
        if File::open(&path).is_ok() {
            return true;
        }
    }

    false
}

fn main() -> Result<()> {
    let instance: HINSTANCE = unsafe { GetModuleHandleA(None)?.into() };
    let args: Vec<String> = env::args().collect();

    if args.len() > 1 {
        // launch the sim with the given cmdline
        let window = create_window(instance, 640, 480)?;
        start_sim(window, &args[1..].join(" "))?;
        return Ok(());
    }

    if !cd_check() {
        unsafe {
            MessageBoxA(
                HWND::default(),
                s!("You must insert the game's CD into your CD-ROM drive."),
                s!("REMECH 2"),
                MB_ICONERROR,
            )
        };
        bail!("You must insert the game's CD into your CD-ROM drive.");
    }

    let window = create_window(instance, 640, 480)?;
    let mut result = start_shell(window, "intro")?;

    loop {
        if result == 255 {
            return Ok(());
        }

        let cmd_line = {
            let mut buffer = vec![];
            let mut file = BufReader::new(File::open("mw2prm.cfg")?);
            file.seek(SeekFrom::Start(280))?;
            for byte in file.bytes() {
                let byte = byte?;
                if byte == 0 {
                    break;
                }
                buffer.push(byte);
            }
            String::from_utf8_lossy(&buffer).to_string()
        };

        result = start_sim(window, &cmd_line)?;

        if result == 255 {
            return Ok(());
        }

        result = start_shell(window, "sim")?;
    }
}
