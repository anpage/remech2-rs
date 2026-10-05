use std::ffi::{c_char, c_void};
use std::ptr::null_mut;
use std::sync::Mutex;
use std::time::{Duration, Instant};

use remech2_sys::shell::{self, AudioSample, TMPackDataBase};

use super::{Campaign, Screen, ScreenArgs, ShellMsg, allocate, delete, run};

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

            let mouse = shell::g_mouseState.as_ref()?;

            let hit = shell::ButtonMenu_HitTest(shell::g_mainMenu, mouse.m_x, mouse.m_y);
            if mouse.m_leftPressed != 1 {
                return None;
            }

            // There's an unused ID 3 for the "EXIT" button at the bottom. Hidden in the Win95 port.
            // TODO: Bring it back?
            match hit {
                0 => {
                    args.set_campaign(Campaign::TrialsOfGrievance);
                    Some(ShellMsg::TRIAL_SETUP)
                }
                1 => {
                    args.set_campaign(Campaign::Wolf);
                    Some(ShellMsg::LANDING)
                }
                2 => {
                    args.set_campaign(Campaign::JadeFalcon);
                    Some(ShellMsg::LANDING)
                }
                _ => None,
            }
        }
    }

    fn teardown(&mut self, _args: &mut ScreenArgs) {
        unsafe {
            shell::CloseAllVideos();

            delete(
                &raw mut shell::g_mainMenu,
                shell::ButtonMenu_ButtonMenu_destructor,
            );
            delete(
                &raw mut shell::g_mainMenuMusic,
                shell::AudioSample_AudioSample_destructor,
            );
            delete(
                &raw mut shell::g_mainMenuIntro,
                shell::AudioSample_AudioSample_destructor,
            );

            shell::g_mainMenuMusicStarted = 0;
        }
    }
}

const FADE_TICK: Duration = Duration::from_micros(50);

impl MainMenu {
    /// Fades in the fire loop once the "MechWarrior: Choose Your Clan" bit ends.
    unsafe fn update_ambient(&mut self, db: *mut TMPackDataBase) {
        unsafe {
            if shell::g_mainMenuMusicStarted != 0 {
                if let Some((start, done)) = &mut self.fade_clock {
                    let due = start.elapsed().as_nanos() / FADE_TICK.as_nanos();
                    for _ in *done..due {
                        shell::AudioSample_DoFade(shell::g_mainMenuMusic);
                    }
                    *done = due;
                }
                return;
            }
            if shell::AudioSample_IsPlaying(shell::g_mainMenuIntro) != 0 {
                return;
            }

            let mut data: *mut c_void = null_mut();
            let mut size = 0;
            shell::TMPackDataBase_GetDBItem(db, AMBIENT_FIRE_DB_ITEM, &mut data, &mut size);

            let sample = allocate::<AudioSample>();
            if sample.is_null() {
                return;
            }
            shell::AudioSample_AudioSample(sample, shell::g_audioSubsystem, data, size as u32);
            shell::g_mainMenuMusic = sample;

            shell::AudioSample_EnableLoop(sample);
            shell::AudioSample_Start(sample);
            shell::AudioSample_SetFade(sample, 500, 1000, 0, 30);
            shell::g_mainMenuMusicStarted = 1;
            self.fade_clock = Some((Instant::now(), 0));
        }
    }
}

static STATE: Mutex<Option<MainMenu>> = Mutex::new(None);

#[unsafe(export_name = "MainMenuCallback")]
pub unsafe extern "C" fn main_menu(
    db: *mut TMPackDataBase,
    campaign: *mut i32,
    pilot_chosen: *mut u8,
    scenario: *mut *mut c_char,
    msg: i32,
) {
    unsafe {
        run(
            &STATE,
            ScreenArgs {
                db,
                campaign,
                pilot_chosen,
                scenario,
            },
            msg as u32,
        );
    }
}
