use std::ffi::{c_char, c_void};
use std::ptr::null_mut;
use std::sync::Mutex;
use std::time::{Duration, Instant};

use binding::{game_fns, globals, macros::hook, patches};

use super::{
    ALLOCATE, DEALLOCATE, FREE_ANIMATIONS, G_MOUSE_STATE, GET_DB_ITEM, Screen, ScreenArgs,
    ShellMsg, ShellState, run,
};
use crate::shell::MODULE;

globals!(
    static G_BUTTONS: *mut c_void = 0x0006ae74;
    static G_AMBIENT_FIRE_SAMPLE: *mut c_void = 0x0006ae78;
    static G_MECHWARRIOR_SAMPLE: *mut c_void = 0x0006ae7c;
    static G_AMBIENT_STARTED: i32 = 0x0006ae80;
    static G_AUDIO_SUBSYSTEM: *mut c_void = 0x000711fc;
);

game_fns!(
    static AUDIO_SAMPLE_NEW: unsafe extern "thiscall" fn(
        *mut c_void,
        *mut c_void,
        *mut c_void,
        i32,
    ) -> *mut c_void = 0x0003d419;
    static AUDIO_SAMPLE_DROP: unsafe extern "thiscall" fn(*mut c_void) = 0x0003d50f;
    static AUDIO_SAMPLE_START: unsafe extern "thiscall" fn(*mut c_void) = 0x0003d6bb;
    static AUDIO_SAMPLE_ENABLE_LOOP: unsafe extern "thiscall" fn(*mut c_void) = 0x0003d67f;
    static AUDIO_SAMPLE_SET_FADE: unsafe extern "thiscall" fn(*mut c_void, i32, i32, i32, i32) =
        0x0003d561;
    static AUDIO_SAMPLE_DO_FADE: unsafe extern "thiscall" fn(*mut c_void) = 0x0003d5ca;
    static AUDIO_SAMPLE_IS_PLAYING: unsafe extern "thiscall" fn(*mut c_void) -> u32 = 0x0003d77e;
    static BUTTONS_HIT_TEST: unsafe extern "thiscall" fn(*mut c_void, i32, i32) -> i32 = 0x000489e9;
    static BUTTONS_DROP: unsafe extern "fastcall" fn(*mut c_void) = 0x0004883e;
);

const AMBIENT_FIRE_DB_ITEM: i32 = 74;

#[derive(Default)]
struct MainMenu {
    /// When the fire loop started, and how many `DoFade` calls it has had.
    fade_clock: Option<(Instant, u128)>,
}

impl Screen for MainMenu {
    const ID: ShellMsg = ShellMsg::MAIN_MENU;

    fn tick(&mut self, args: &mut ScreenArgs) -> Option<ShellMsg> {
        unsafe {
            self.update_ambient(args.db);

            let mouse = G_MOUSE_STATE.get().as_ref()?;

            let hit = (BUTTONS_HIT_TEST.get())(G_BUTTONS.get(), mouse.pos_x, mouse.pos_y);
            if mouse.left_pressed.0 != 1 {
                return None;
            }

            // There's an unused ID 3 for the "EXIT" button at the bottom. Hidden in the Win95 port.
            // TODO: Bring it back?
            match hit {
                0 => {
                    args.set_shell_state(ShellState::Trial);
                    Some(ShellMsg::TRIAL_SETUP)
                }
                1 => {
                    args.set_shell_state(ShellState::Wolf);
                    Some(ShellMsg::LANDING)
                }
                2 => {
                    args.set_shell_state(ShellState::JadeFalcon);
                    Some(ShellMsg::LANDING)
                }
                _ => None,
            }
        }
    }

    fn teardown(&mut self, _args: &mut ScreenArgs) {
        unsafe {
            (FREE_ANIMATIONS.get())();

            let buttons = G_BUTTONS.get();
            if !buttons.is_null() {
                (BUTTONS_DROP.get())(buttons);
                (DEALLOCATE.get())(buttons);
            }
            G_BUTTONS.set(null_mut());

            for sample in [&G_AMBIENT_FIRE_SAMPLE, &G_MECHWARRIOR_SAMPLE] {
                let p = sample.get();
                if !p.is_null() {
                    (AUDIO_SAMPLE_DROP.get())(p);
                    (DEALLOCATE.get())(p);
                }
                sample.set(null_mut());
            }

            G_AMBIENT_STARTED.set(0);
        }
    }
}

const FADE_TICK: Duration = Duration::from_micros(50);

impl MainMenu {
    /// Fades in the fire loop once the "MechWarrior: Choose Your Clan" bit ends.
    unsafe fn update_ambient(&mut self, db: *mut c_void) {
        unsafe {
            if G_AMBIENT_STARTED.get() != 0 {
                if let Some((start, done)) = &mut self.fade_clock {
                    let due = start.elapsed().as_nanos() / FADE_TICK.as_nanos();
                    let sample = G_AMBIENT_FIRE_SAMPLE.get();
                    for _ in *done..due {
                        (AUDIO_SAMPLE_DO_FADE.get())(sample);
                    }
                    *done = due;
                }
                return;
            }
            if (AUDIO_SAMPLE_IS_PLAYING.get())(G_MECHWARRIOR_SAMPLE.get()) & 0xff != 0 {
                return;
            }

            let mut data = null_mut();
            let mut size = 0;
            (GET_DB_ITEM.get())(db, AMBIENT_FIRE_DB_ITEM, &mut data, &mut size);

            let sample = (ALLOCATE.get())(0x2c);
            if sample.is_null() {
                return;
            }
            let sample = (AUDIO_SAMPLE_NEW.get())(sample, G_AUDIO_SUBSYSTEM.get(), data, size);
            G_AMBIENT_FIRE_SAMPLE.set(sample);

            (AUDIO_SAMPLE_ENABLE_LOOP.get())(sample);
            (AUDIO_SAMPLE_START.get())(sample);
            (AUDIO_SAMPLE_SET_FADE.get())(sample, 500, 1000, 0, 30);
            G_AMBIENT_STARTED.set(1);
            self.fade_clock = Some((Instant::now(), 0));
        }
    }
}

static STATE: Mutex<Option<MainMenu>> = Mutex::new(None);

#[hook(rva = 0x0003dd89)]
unsafe extern "cdecl" fn main_menu(
    db: *mut c_void,
    shell_state: *mut i32,
    state_byte: *mut u8,
    state_word: *mut *mut c_char,
    msg: u32,
) {
    unsafe {
        run(
            &STATE,
            ScreenArgs {
                db,
                shell_state,
                state_byte,
                state_word,
            },
            msg,
        );
    }
}

patches!(
    pub(in crate::shell) static PATCHES = [
        hook main_menu,
    ];
);
