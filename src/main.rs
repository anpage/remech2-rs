#![feature(layout_for_ptr)]
#![feature(c_variadic)]

use anyhow::Result;
use std::{
    env,
    fs::File,
    io::{BufReader, Read, Seek, SeekFrom},
};
use tracing::Level;
use tracing_subscriber::{filter, prelude::*};
use windows::{
    Win32::{
        Foundation::*, Graphics::Gdi::*, System::LibraryLoader::GetModuleHandleA,
        UI::WindowsAndMessaging::*,
    },
    core::{PCSTR, s},
};

mod ail;
mod common;
mod hooker;
mod launcher;
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

pub static mut WINDOW_WIDTH: i32 = 640;
pub static mut WINDOW_HEIGHT: i32 = 480;

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
            WM_SIZE => {
                let width = (lparam.0 as i32) & 0xFFFF;
                let height = (lparam.0 as i32 >> 16) & 0xFFFF;

                tracing::debug!("Window resized: width = {}, height = {}", width, height);

                if width != WINDOW_WIDTH || height != WINDOW_HEIGHT {
                    WINDOW_WIDTH = width;
                    WINDOW_HEIGHT = height;
                }
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

fn create_window(width: i32, height: i32) -> Result<(HWND, HINSTANCE)> {
    unsafe {
        let instance: HINSTANCE = GetModuleHandleA(None)?.into();

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
            hCursor: LoadCursorW(None, IDC_ARROW)?,
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
            Some(instance),
            None,
        )?;

        SetMenu(window, None)?;
        let _ = ShowWindow(window, SW_SHOWDEFAULT);
        let _ = UpdateWindow(window);

        Ok((window, instance))
    }
}

fn start_launcher(window: HWND, instance: HINSTANCE) -> Result<()> {
    let mut launcher = launcher::Launcher::new(window, instance)?;
    launcher.launch()?;
    Ok(())
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

fn main() -> Result<()> {
    let filter = filter::Targets::new().with_target("remech2", Level::DEBUG);
    tracing_subscriber::registry()
        .with(tracing_subscriber::fmt::layer())
        .with(filter)
        .init();

    let args: Vec<String> = env::args().collect();

    let (window, instance) = create_window(640, 480)?;

    start_launcher(window, instance)?;

    if args.len() > 1 {
        // launch the sim with the given cmdline
        start_sim(window, &args[1..].join(" "))?;
        return Ok(());
    }

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
            let cmd_line = String::from_utf8_lossy(&buffer).to_string();
            format!("{} {}", cmd_line, "/V=5")
        };

        result = start_sim(window, &cmd_line)?;

        if result == 255 {
            return Ok(());
        }

        result = start_shell(window, "sim")?;
    }
}
