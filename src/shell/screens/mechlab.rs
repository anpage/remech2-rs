use std::ffi::{CStr, c_char};
use std::fs;
use std::ptr::null_mut;
use std::sync::Mutex;

use remech2_sys::shell::{self, ScreenField, TMPackDataBase};
use tracing::warn;

use super::{Campaign, Screen, ScreenArgs, ShellMsg, click_field, delete, run};
use crate::files;
use crate::shell::overlay::confirm;

fn variant_rows() -> *mut ScreenField {
    (&raw mut shell::g_mechBayFields).cast()
}

fn new_variant_rows() -> *mut ScreenField {
    (&raw mut shell::g_customizeFields).cast()
}

fn customize_rows() -> *mut ScreenField {
    (&raw mut shell::g_engineFields).cast()
}

const BTN_EXIT_LAB: i32 = 0;
const BTN_STAR_CONFIG: i32 = 1;
const BTN_NEXT_CHASSIS: i32 = 2;
const BTN_PREV_CHASSIS: i32 = 3;
const BTN_NEXT_VARIANT: i32 = 4;
const BTN_PREV_VARIANT: i32 = 5;
const BTN_CUSTOMIZE: i32 = 6;
const BTN_ACCEPT_MECH: i32 = 7;
const BTN_SAVE: i32 = 8;
const BTN_ABORT: i32 = 9;
const BTN_DELETE: i32 = 10;

/// Every button that is active while browsing and hidden while customizing
const BROWSE_BUTTONS: [i32; 8] = [
    BTN_EXIT_LAB,
    BTN_STAR_CONFIG,
    BTN_NEXT_CHASSIS,
    BTN_PREV_CHASSIS,
    BTN_NEXT_VARIANT,
    BTN_PREV_VARIANT,
    BTN_CUSTOMIZE,
    BTN_ACCEPT_MECH,
];

const ANIM_GRID: i32 = 0;
const ANIM_STAR_CONFIG: i32 = 5;
const ANIM_NEXT_VARIANT: i32 = 6;
const ANIM_PREV_VARIANT: i32 = 7;
const ANIM_NEXT_CHASSIS: i32 = 8;
const ANIM_PREV_CHASSIS: i32 = 9;
const ANIM_MACHINERY: [i32; 4] = [0xa, 0xb, 0xc, 0xd];
const ANIM_MACHINERY_CLOSE: i32 = 0xe;
/// The selected mech spinning on the platform
const ANIM_MECH: i32 = 0x10;

const ANIM_ENDED: i32 = 0x1;
const ANIM_PAUSED: i32 = 0x20;
const ANIM_FREE: i32 = 0x4000_0000;

const FIRST_USER_VARIANT: usize = 100;
/// Later chassis are trials-only and have no custom variants
const CUSTOMIZABLE_CHASSIS: i32 = 15;

/// What prompt the screen is waiting on, if any
#[derive(Default, PartialEq)]
enum Prompt {
    #[default]
    None,
    Delete,
    Alert,
}

#[derive(Default)]
struct Mechbay {
    prompt: Prompt,
}

impl Screen for Mechbay {
    const ID: ShellMsg = ShellMsg::MECHBAY;

    fn tick(&mut self, args: &mut ScreenArgs) -> Option<ShellMsg> {
        unsafe {
            match self.prompt {
                Prompt::Delete => {
                    let delete = confirm::answer()?;
                    confirm::close();
                    self.prompt = Prompt::None;
                    if delete {
                        self.delete_variant();
                    }
                    return None;
                }
                Prompt::Alert => {
                    confirm::answer()?;
                    confirm::close();
                    self.prompt = Prompt::None;
                    return None;
                }
                Prompt::None => {}
            }

            // TODO: Maybe consider bringing back quick tips

            let mouse = shell::g_mouseState.as_ref()?;
            let (pos_x, pos_y) = (mouse.m_x, mouse.m_y);
            let clicked = mouse.m_leftPressed == 1;
            let held = mouse.m_leftDown == 1;

            if clicked {
                self.click_rows(pos_x, pos_y);
            }

            let hit = shell::ButtonMenu_HitTest(shell::g_mechBayMenu, pos_x, pos_y);

            let trials = args.campaign() == Some(Campaign::TrialsOfGrievance);
            if trials {
                shell::SetVideoFlags(ANIM_STAR_CONFIG, ANIM_PAUSED, ANIM_PAUSED);
            } else {
                shell::SetVideoFlags(ANIM_STAR_CONFIG, ANIM_ENDED, ANIM_ENDED);
            }
            for slot in [
                ANIM_NEXT_VARIANT,
                ANIM_PREV_VARIANT,
                ANIM_NEXT_CHASSIS,
                ANIM_PREV_CHASSIS,
            ] {
                shell::SetVideoFlags(slot, ANIM_PAUSED, ANIM_PAUSED);
            }

            match hit {
                BTN_EXIT_LAB => clicked.then_some(ShellMsg::MISSION_SELECT),
                BTN_STAR_CONFIG => {
                    if trials {
                        shell::ShowVideo(ANIM_STAR_CONFIG);
                    } else {
                        shell::SetVideoFlags(ANIM_STAR_CONFIG, ANIM_ENDED, 0);
                    }

                    if !clicked {
                        return None;
                    }

                    shell::AudioSample_Start(shell::g_acceptSound);
                    if shell::g_pickingStarMech == 0 || register_selected() {
                        return Some(ShellMsg::MECH_CONFIG);
                    }

                    self.keshik_max();

                    None
                }
                BTN_NEXT_CHASSIS => {
                    if held {
                        shell::ShowVideo(ANIM_NEXT_CHASSIS);
                    }

                    if clicked {
                        self.change_chassis(shell::NextChassis);
                    }

                    None
                }
                BTN_PREV_CHASSIS => {
                    if held {
                        shell::ShowVideo(ANIM_PREV_CHASSIS);
                    }

                    if clicked {
                        self.change_chassis(shell::PreviousChassis);
                    }

                    None
                }
                BTN_NEXT_VARIANT => {
                    if held {
                        shell::ShowVideo(ANIM_NEXT_VARIANT);
                    }

                    if clicked {
                        self.change_variant(shell::NextVariant);
                    }

                    None
                }
                BTN_PREV_VARIANT => {
                    if held {
                        shell::ShowVideo(ANIM_PREV_VARIANT);
                    }

                    if clicked {
                        self.change_variant(shell::PreviousVariant);
                    }

                    None
                }
                BTN_CUSTOMIZE => {
                    if clicked {
                        self.customize();
                    }

                    None
                }
                BTN_ACCEPT_MECH => {
                    if !clicked {
                        return None;
                    }

                    if register_selected() {
                        return Some(ShellMsg(shell::g_mechBayWParam as u32));
                    }

                    self.keshik_max();

                    None
                }
                BTN_SAVE | BTN_ABORT => {
                    // Keep the customzie screen open if the save is rejected
                    if clicked && (hit != BTN_SAVE || shell::SaveUserVariant() != 0) {
                        self.browse();
                    }

                    None
                }
                BTN_DELETE => {
                    if clicked {
                        confirm::open(&["Delete this 'Mech?", "Are you sure?"]);
                        self.prompt = Prompt::Delete;
                    }

                    None
                }
                _ => None,
            }
        }
    }

    fn teardown(&mut self, _args: &mut ScreenArgs) {
        unsafe {
            if self.prompt != Prompt::None {
                confirm::close();
            }

            shell::HideFields(shell::g_screenFields);
            shell::HideFields(shell::g_componentFields);
            shell::CloseAllVideos();

            for sample in [
                &raw mut shell::g_mechBayAmbience,
                &raw mut shell::g_chassisNameSound,
                &raw mut shell::g_locationSound,
                &raw mut shell::g_acceptSound,
                &raw mut shell::g_variantSound,
            ] {
                delete(sample, shell::AudioSample_AudioSample_destructor);
            }

            shell::g_unk0x1006178c = 1;

            delete(
                &raw mut shell::g_mechBayMenu,
                shell::ButtonMenu_ButtonMenu_destructor,
            );
        }
    }
}

impl Mechbay {
    /// Runs the handler of whatever row was clicked
    unsafe fn click_rows(&self, x: i32, y: i32) {
        unsafe {
            let rows = shell::g_screenFields;
            if let Some(row) = shell::FindFieldAt(rows, x, y).as_mut() {
                click_field(row);
                return;
            }

            let extra = shell::g_componentFields;
            if extra.is_null() {
                return;
            }
            let Some(row) = shell::FindFieldAt(extra, x, y).as_mut() else {
                return;
            };
            click_field(row);

            // The second group's rows can change what the first one reads.
            shell::RedrawFields(extra);
            shell::RedrawFields(rows);
        }
    }

    unsafe fn change_chassis(&self, step: unsafe extern "C" fn()) {
        unsafe {
            step();
            shell::HideFields(shell::g_componentFields);
            shell::g_componentFields = null_mut();
            shell::RedrawFields(shell::g_screenFields);

            let buttons = shell::g_mechBayMenu;
            if shell::g_selectedChassis >= CUSTOMIZABLE_CHASSIS {
                shell::ButtonMenu_DisableButton(buttons, BTN_CUSTOMIZE);
            } else if shell::g_pickingStarMech == 0 {
                shell::ButtonMenu_EnableButton(buttons, BTN_CUSTOMIZE);
            }

            shell::ButtonMenu_DisableButton(buttons, BTN_DELETE);
        }
    }

    unsafe fn change_variant(&self, step: unsafe extern "C" fn()) {
        unsafe {
            shell::AudioSample_Start(shell::g_variantSound);
            step();
            self.update_delete_button();
        }
    }

    /// Only the user variants can be deleted
    unsafe fn update_delete_button(&self) {
        unsafe {
            let buttons = shell::g_mechBayMenu;
            if shell::g_selectedVariant < FIRST_USER_VARIANT as i32 {
                shell::ButtonMenu_DisableButton(buttons, BTN_DELETE);
            } else {
                shell::ButtonMenu_EnableButton(buttons, BTN_DELETE);
            }
        }
    }

    /// Swaps in the buttons and rows for the mech customization UI
    unsafe fn customize(&mut self) {
        unsafe {
            let Some(slot) = free_user_slot() else {
                self.alert(&["Error: Too many mechs", "of this variant to save."]);
                return;
            };
            name_new_variant(slot);

            shell::SetVideoFlags(ANIM_GRID, ANIM_ENDED, ANIM_ENDED);
            shell::SetVideoFlags(ANIM_MECH, ANIM_FREE, ANIM_FREE);

            shell::HideFields(shell::g_screenFields);
            shell::g_screenFields = new_variant_rows();
            shell::ShowFields(shell::g_screenFields);
            shell::HideFields(shell::g_componentFields);
            shell::g_componentFields = customize_rows();
            shell::ShowFields(shell::g_componentFields);

            let buttons = shell::g_mechBayMenu;
            for id in BROWSE_BUTTONS.into_iter().chain([BTN_DELETE]) {
                shell::ButtonMenu_DisableButton(buttons, id);
            }
            for id in [BTN_SAVE, BTN_ABORT] {
                shell::ButtonMenu_EnableButton(buttons, id);
            }

            // TODO: Maybe consider bringing back quick tips
        }
    }

    /// Leaves customize mode
    unsafe fn browse(&self) {
        unsafe {
            for slot in ANIM_MACHINERY {
                shell::SetVideoFlags(slot, ANIM_PAUSED, ANIM_PAUSED);
            }
            shell::ShowVideo(ANIM_MACHINERY_CLOSE);
            shell::LoadChassis();

            shell::HideFields(shell::g_componentFields);
            shell::g_componentFields = null_mut();
            shell::HideFields(shell::g_screenFields);
            shell::g_screenFields = variant_rows();
            shell::ShowFields(shell::g_screenFields);

            shell::SetVideoFlags(ANIM_GRID, ANIM_ENDED, 0);
            shell::ShowVideo(ANIM_MECH);

            let buttons = shell::g_mechBayMenu;
            for id in BROWSE_BUTTONS {
                shell::ButtonMenu_EnableButton(buttons, id);
            }
            for id in [BTN_SAVE, BTN_ABORT] {
                shell::ButtonMenu_DisableButton(buttons, id);
            }
            self.update_delete_button();
        }
    }

    /// Shows the tonnage check message
    fn keshik_max(&mut self) {
        self.alert(&[
            "'Mech exceeds",
            "Keshik Defined Maximum Tonnage (KDMT)",
            "for mission.",
        ]);
    }

    /// Shows a dialog box with "Ok" as the only button
    fn alert(&mut self, lines: &[&str]) {
        confirm::alert(lines);
        self.prompt = Prompt::Alert;
    }

    /// Deletes the selected variant's file and moves off the emptied slot
    unsafe fn delete_variant(&self) {
        unsafe {
            let slot = &raw mut (*variant_files())[shell::g_selectedVariant as usize];
            match CStr::from_ptr((*slot).as_ptr()).to_str() {
                Ok(name) => {
                    let path = files::resolve_user(&format!("mek\\{name}.mek"));
                    if let Err(e) = fs::remove_file(&path) {
                        warn!("mechbay: deleting {}: {e}", path.display());
                    }
                }
                Err(e) => warn!("mechbay: variant filename is not UTF-8: {e}"),
            }
            (*slot)[0] = 0;

            shell::NextVariant();
            self.update_delete_button();
        }
    }
}

const VARIANT_SLOTS: usize = 200;

fn variant_files() -> *mut [[c_char; 13]; VARIANT_SLOTS] {
    &raw mut shell::g_variantFiles
}

/// Registers the selected variant against the mission. `false` means it's over the tonnage limit.
unsafe fn register_selected() -> bool {
    unsafe {
        let filename = (*variant_files())[shell::g_selectedVariant as usize].as_mut_ptr();
        shell::SetStarMech(-1, filename, null_mut()) != 0
    }
}

/// The first free slot for user variants
unsafe fn free_user_slot() -> Option<usize> {
    unsafe {
        let filenames = variant_files();
        (FIRST_USER_VARIANT..VARIANT_SLOTS).find(|&i| (*filenames)[i][0] == 0)
    }
}

/// Names the new variant after the slot it will take: `User Variant #1` and up
unsafe fn name_new_variant(slot: usize) {
    unsafe {
        let name = format!("User Variant #{}", slot - FIRST_USER_VARIANT + 1);
        // The longest the format can produce is `User Variant #100`.
        let dst = &raw mut shell::g_variant.m_variantName;
        debug_assert!(name.len() < size_of_val(&*dst));
        let dst = dst.cast::<u8>();
        std::ptr::copy_nonoverlapping(name.as_ptr(), dst, name.len());
        *dst.add(name.len()) = 0;
    }
}

static STATE: Mutex<Option<Mechbay>> = Mutex::new(None);

#[unsafe(export_name = "MechBayCallback")]
pub unsafe extern "C" fn mechbay(
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
