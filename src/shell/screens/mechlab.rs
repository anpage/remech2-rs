use std::ffi::{CStr, CString, c_char, c_void};
use std::fs;
use std::path::Path;
use std::ptr::{null, null_mut};
use std::sync::Mutex;

use binding::{game_fns, globals, macros::hook, patches};
use tracing::warn;

use super::{
    BUTTON_DISABLE, BUTTON_ENABLE, BUTTONS_DROP, BUTTONS_HIT_TEST, CLICKABLES_HIDE,
    CLICKABLES_HIT_TEST, CLICKABLES_REBUILD, CLICKABLES_SHOW, Campaign, Clickable, DEALLOCATE,
    FREE_ANIMATIONS, REGISTER_MECH_VARIANT, Screen, ScreenArgs, ShellMsg, run,
};
use crate::shell::MODULE;
use crate::shell::audio::{AUDIO_SAMPLE_DROP, AUDIO_SAMPLE_START};
use crate::shell::drawmode::confirm;
use crate::shell::drawmode::hooks::G_CURRENT_MOUSE_STATE;

globals!(
    static G_BUTTONS: *mut c_void = 0x00061778;
    /// The variant panel or the name editor while customizing
    static G_ROWS: *mut Clickable = 0x0005de2c;
    /// Only set while customizing
    static G_EXTRA_ROWS: *mut Clickable = 0x0005de28;
    /// The selected variant's name and stats
    static G_VARIANT_ROWS: Clickable = 0x00060d20;
    /// The new variant's name
    static G_NEW_VARIANT_ROWS: Clickable = 0x00060698;
    /// `G_EXTRA_ROWS` while customizing
    static G_CUSTOMIZE_ROWS: Clickable = 0x0005de80;
    /// The selected chassis
    static G_CHASSIS: i32 = 0x0006176c;
    /// The selected variant.
    /// `0..100` are the stock ones and `100..200` are the user's.
    static G_VARIANT: i32 = 0x0007cc80;
    /// The label where a new variant's name is stored
    static G_VARIANT_NAME: [c_char; 20] = 0x0005c740;
    /// Set when the screen was entered from the mech config with a mech being edited
    static G_EDITING: i32 = 0x00079aa0;
    /// The message passed when `ACCEPT MECH` is clicked
    static G_EXIT_MSG: u32 = 0x00061780;
    static G_ELECTRIC_SAMPLE: *mut c_void = 0x0006177c;
    static G_MECH_NAME_SAMPLE: *mut c_void = 0x00061770;
    static G_KERCHUNK_SAMPLE: *mut c_void = 0x0005de78;
    static G_THUNK_SAMPLE: *mut c_void = 0x00079ac8;
    static G_CLICK_SAMPLE: *mut c_void = 0x00079d18;
    static G_UNREGISTER_PENDING: i32 = 0x0006178c;
    static G_MECH_VARIANT_FILENAMES: [[c_char; 13]; 200] = 0x00079d80;
    static G_PRJ_OBJECT: c_void = 0x00071230;
);

game_fns!(
    static NEXT_CHASSIS: unsafe extern "cdecl" fn() = 0x0000cce5;
    static PREV_CHASSIS: unsafe extern "cdecl" fn() = 0x0000cd2a;
    static NEXT_VARIANT: unsafe extern "cdecl" fn() = 0x0000cd65;
    static PREV_VARIANT: unsafe extern "cdecl" fn() = 0x0000ce50;
    /// Writes the edited mech to `mek\…usr.mek` and takes the next free user slot.
    /// Returns `0` when the spec is invalid
    static SAVE_VARIANT: unsafe extern "cdecl" fn() -> i32 = 0x0000cf4c;
    /// Restarts the chassis preview video and reloads its build list
    static SHOW_CHASSIS: unsafe extern "cdecl" fn() = 0x0000ca74;
    /// `(slot, mask, value)`: `flags = flags & !mask | mask & value` on an animation slot
    static ANIM_SET_FLAGS: unsafe extern "cdecl" fn(i32, u32, u32) = 0x00016cc0;
    /// Unpauses a paused animation slot and marks it dirty
    static ANIM_RESUME: unsafe extern "cdecl" fn(i32) = 0x00016d27;
    /// Loads a named file out of MW2.PRJ
    static LOAD_FILE_FROM_PRJ: unsafe extern "thiscall" fn(*mut c_void, *const c_char, i32) -> i32 =
        0x0002e346;
);

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

const ANIM_ENDED: u32 = 0x1;
const ANIM_PAUSED: u32 = 0x20;
const ANIM_FREE: u32 = 0x4000_0000;

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

            let mouse = G_CURRENT_MOUSE_STATE.get().as_ref()?;
            let (pos_x, pos_y) = (mouse.pos_x, mouse.pos_y);
            let clicked = mouse.left_pressed.0 == 1;
            let held = mouse.left_down.0 == 1;

            if clicked {
                self.click_rows(pos_x, pos_y);
            }

            let hit = (BUTTONS_HIT_TEST.get())(G_BUTTONS.get(), pos_x, pos_y);

            let trials = args.campaign() == Some(Campaign::TrialsOfGrievance);
            if trials {
                (ANIM_SET_FLAGS.get())(ANIM_STAR_CONFIG, ANIM_PAUSED, ANIM_PAUSED);
            } else {
                (ANIM_SET_FLAGS.get())(ANIM_STAR_CONFIG, ANIM_ENDED, ANIM_ENDED);
            }
            for slot in [
                ANIM_NEXT_VARIANT,
                ANIM_PREV_VARIANT,
                ANIM_NEXT_CHASSIS,
                ANIM_PREV_CHASSIS,
            ] {
                (ANIM_SET_FLAGS.get())(slot, ANIM_PAUSED, ANIM_PAUSED);
            }

            match hit {
                BTN_EXIT_LAB => clicked.then_some(ShellMsg::MISSION_SELECT),
                BTN_STAR_CONFIG => {
                    if trials {
                        (ANIM_RESUME.get())(ANIM_STAR_CONFIG);
                    } else {
                        (ANIM_SET_FLAGS.get())(ANIM_STAR_CONFIG, ANIM_ENDED, 0);
                    }

                    if !clicked {
                        return None;
                    }

                    (AUDIO_SAMPLE_START.get())(G_THUNK_SAMPLE.get());
                    if G_EDITING.get() == 0 || register_selected() {
                        return Some(ShellMsg::MECH_CONFIG);
                    }

                    self.keshik_max();

                    None
                }
                BTN_NEXT_CHASSIS => {
                    if held {
                        (ANIM_RESUME.get())(ANIM_NEXT_CHASSIS);
                    }

                    if clicked {
                        self.change_chassis(NEXT_CHASSIS.get());
                    }

                    None
                }
                BTN_PREV_CHASSIS => {
                    if held {
                        (ANIM_RESUME.get())(ANIM_PREV_CHASSIS);
                    }

                    if clicked {
                        self.change_chassis(PREV_CHASSIS.get());
                    }

                    None
                }
                BTN_NEXT_VARIANT => {
                    if held {
                        (ANIM_RESUME.get())(ANIM_NEXT_VARIANT);
                    }

                    if clicked {
                        self.change_variant(NEXT_VARIANT.get());
                    }

                    None
                }
                BTN_PREV_VARIANT => {
                    if held {
                        (ANIM_RESUME.get())(ANIM_PREV_VARIANT);
                    }

                    if clicked {
                        self.change_variant(PREV_VARIANT.get());
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
                        return Some(ShellMsg(G_EXIT_MSG.get()));
                    }

                    self.keshik_max();

                    None
                }
                BTN_SAVE | BTN_ABORT => {
                    // Keep the customzie screen open if the save is rejected
                    if clicked && (hit != BTN_SAVE || (SAVE_VARIANT.get())() != 0) {
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

            (CLICKABLES_HIDE.get())(G_ROWS.get());
            (CLICKABLES_HIDE.get())(G_EXTRA_ROWS.get());
            (FREE_ANIMATIONS.get())();

            for sample in [
                &G_ELECTRIC_SAMPLE,
                &G_MECH_NAME_SAMPLE,
                &G_KERCHUNK_SAMPLE,
                &G_THUNK_SAMPLE,
                &G_CLICK_SAMPLE,
            ] {
                let p = sample.get();
                if !p.is_null() {
                    (AUDIO_SAMPLE_DROP.get())(p);
                    (DEALLOCATE.get())(p);
                }
                sample.set(null_mut());
            }

            G_UNREGISTER_PENDING.set(1);

            let buttons = G_BUTTONS.get();
            if !buttons.is_null() {
                (BUTTONS_DROP.get())(buttons);
                (DEALLOCATE.get())(buttons);
            }
            G_BUTTONS.set(null_mut());
        }
    }
}

impl Mechbay {
    /// Runs the handler of whatever row was clicked
    unsafe fn click_rows(&self, x: i32, y: i32) {
        unsafe {
            let rows = G_ROWS.get();
            if let Some(row) = (CLICKABLES_HIT_TEST.get())(rows, x, y).as_mut() {
                row.click();
                return;
            }

            let extra = G_EXTRA_ROWS.get();
            if extra.is_null() {
                return;
            }
            let Some(row) = (CLICKABLES_HIT_TEST.get())(extra, x, y).as_mut() else {
                return;
            };
            row.click();

            // The second group's rows can change what the first one reads.
            (CLICKABLES_REBUILD.get())(extra);
            (CLICKABLES_REBUILD.get())(rows);
        }
    }

    unsafe fn change_chassis(&self, step: unsafe extern "cdecl" fn()) {
        unsafe {
            step();
            (CLICKABLES_HIDE.get())(G_EXTRA_ROWS.get());
            G_EXTRA_ROWS.set(null_mut());
            (CLICKABLES_REBUILD.get())(G_ROWS.get());

            let buttons = G_BUTTONS.get();
            if G_CHASSIS.get() >= CUSTOMIZABLE_CHASSIS {
                (BUTTON_DISABLE.get())(buttons, BTN_CUSTOMIZE);
            } else if G_EDITING.get() == 0 {
                (BUTTON_ENABLE.get())(buttons, BTN_CUSTOMIZE);
            }

            (BUTTON_DISABLE.get())(buttons, BTN_DELETE);
        }
    }

    unsafe fn change_variant(&self, step: unsafe extern "cdecl" fn()) {
        unsafe {
            (AUDIO_SAMPLE_START.get())(G_CLICK_SAMPLE.get());
            step();
            self.update_delete_button();
        }
    }

    /// Only the user variants can be deleted
    unsafe fn update_delete_button(&self) {
        unsafe {
            let buttons = G_BUTTONS.get();
            if G_VARIANT.get() < FIRST_USER_VARIANT as i32 {
                (BUTTON_DISABLE.get())(buttons, BTN_DELETE);
            } else {
                (BUTTON_ENABLE.get())(buttons, BTN_DELETE);
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

            (ANIM_SET_FLAGS.get())(ANIM_GRID, ANIM_ENDED, ANIM_ENDED);
            (ANIM_SET_FLAGS.get())(ANIM_MECH, ANIM_FREE, ANIM_FREE);

            (CLICKABLES_HIDE.get())(G_ROWS.get());
            G_ROWS.set(G_NEW_VARIANT_ROWS.ptr());
            (CLICKABLES_SHOW.get())(G_ROWS.get());
            (CLICKABLES_HIDE.get())(G_EXTRA_ROWS.get());
            G_EXTRA_ROWS.set(G_CUSTOMIZE_ROWS.ptr());
            (CLICKABLES_SHOW.get())(G_EXTRA_ROWS.get());

            let buttons = G_BUTTONS.get();
            for id in BROWSE_BUTTONS.into_iter().chain([BTN_DELETE]) {
                (BUTTON_DISABLE.get())(buttons, id);
            }
            for id in [BTN_SAVE, BTN_ABORT] {
                (BUTTON_ENABLE.get())(buttons, id);
            }

            // TODO: Maybe consider bringing back quick tips
        }
    }

    /// Leaves customize mode
    unsafe fn browse(&self) {
        unsafe {
            for slot in ANIM_MACHINERY {
                (ANIM_SET_FLAGS.get())(slot, ANIM_PAUSED, ANIM_PAUSED);
            }
            (ANIM_RESUME.get())(ANIM_MACHINERY_CLOSE);
            (SHOW_CHASSIS.get())();

            (CLICKABLES_HIDE.get())(G_EXTRA_ROWS.get());
            G_EXTRA_ROWS.set(null_mut());
            (CLICKABLES_HIDE.get())(G_ROWS.get());
            G_ROWS.set(G_VARIANT_ROWS.ptr());
            (CLICKABLES_SHOW.get())(G_ROWS.get());

            (ANIM_SET_FLAGS.get())(ANIM_GRID, ANIM_ENDED, 0);
            (ANIM_RESUME.get())(ANIM_MECH);

            let buttons = G_BUTTONS.get();
            for id in BROWSE_BUTTONS {
                (BUTTON_ENABLE.get())(buttons, id);
            }
            for id in [BTN_SAVE, BTN_ABORT] {
                (BUTTON_DISABLE.get())(buttons, id);
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
            let slot = &mut (*G_MECH_VARIANT_FILENAMES.ptr())[G_VARIANT.get() as usize];
            match CStr::from_ptr(slot.as_ptr()).to_str() {
                Ok(name) => {
                    let path = Path::new("mek").join(format!("{name}.mek"));
                    if let Err(e) = fs::remove_file(&path) {
                        warn!("mechbay: deleting {}: {e}", path.display());
                    }
                }
                Err(e) => warn!("mechbay: variant filename is not UTF-8: {e}"),
            }
            slot[0] = 0;

            (NEXT_VARIANT.get())();
            self.update_delete_button();
        }
    }
}

/// Registers the selected variant against the mission. `false` means it's over the tonnage limit.
unsafe fn register_selected() -> bool {
    unsafe {
        let filename = (*G_MECH_VARIANT_FILENAMES.ptr())[G_VARIANT.get() as usize].as_ptr();
        (REGISTER_MECH_VARIANT.get())(-1, filename, null()) != 0
    }
}

/// The first free slot for user variants
unsafe fn free_user_slot() -> Option<usize> {
    unsafe {
        let filenames = &*G_MECH_VARIANT_FILENAMES.ptr();
        (FIRST_USER_VARIANT..filenames.len()).find(|&i| filenames[i][0] == 0)
    }
}

/// Names the new variant after the slot it will take: `User Variant #1` and up
unsafe fn name_new_variant(slot: usize) {
    unsafe {
        let name = format!("User Variant #{}", slot - FIRST_USER_VARIANT + 1);
        // The longest the format can produce is `User Variant #100`.
        debug_assert!(name.len() < size_of_val(&*G_VARIANT_NAME.ptr()));
        let dst = G_VARIANT_NAME.ptr().cast::<u8>();
        std::ptr::copy_nonoverlapping(name.as_ptr(), dst, name.len());
        *dst.add(name.len()) = 0;
    }
}

static STATE: Mutex<Option<Mechbay>> = Mutex::new(None);

#[hook(rva = 0x0000db35)]
unsafe extern "cdecl" fn mechbay(
    db: *mut c_void,
    campaign: *mut i32,
    pilot_chosen: *mut u8,
    scenario: *mut *mut c_char,
    msg: u32,
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
            msg,
        );
    }
}

/// Loads the list of mech variants from the MW2.PRJ file and any user variants from the filesystem.
/// Patched to avoid a bug where the game was using an older Win32 API
#[hook(rva = 0x0000c8b8)]
unsafe extern "cdecl" fn load_mech_variant_list(mech_type: *const c_char) {
    unsafe {
        const SLOT_LEN: usize = 13;

        let write_slot = |dst: *mut c_char, name: &[u8]| {
            debug_assert!(name.len() < SLOT_LEN);
            let dst = dst.cast::<u8>();
            std::ptr::write_bytes(dst, 0, SLOT_LEN);
            std::ptr::copy_nonoverlapping(name.as_ptr(), dst, name.len());
        };

        // Create "MEK" folder if it doesn't exist
        if let Err(e) = fs::create_dir_all("MEK") {
            tracing::warn!("load_mech_variant_list: cannot create MEK dir: {e}");
        }

        let Ok(mech_type) = CStr::from_ptr(mech_type).to_str() else {
            tracing::warn!("load_mech_variant_list: non-UTF8 mech_type");
            return;
        };

        // Clear the list
        let filenames = &mut *(G_MECH_VARIANT_FILENAMES.ptr());
        filenames.fill([0; SLOT_LEN as _]);

        // Make sure we have at least the default variant
        write_slot(
            filenames[0].as_mut_ptr(),
            format!("{mech_type}00std").as_bytes(),
        );

        // Load the built-in mech variants from the MW2.PRJ file into the next 99 indices
        for (i, slot) in filenames[1..100].iter_mut().enumerate() {
            let variant = format!("{mech_type}{:02}std", i + 1);
            let c_variant = CString::new(variant.as_str()).expect("no interior NUL");

            let result = (LOAD_FILE_FROM_PRJ.get())(G_PRJ_OBJECT.ptr(), c_variant.as_ptr(), 6);

            if result > -1 {
                write_slot(slot.as_mut_ptr(), variant.as_bytes());
            }
        }

        // Find all user-defined mech variants from the filesystem and load their names into index 100 and higher
        let Ok(files) = fs::read_dir("MEK") else {
            return;
        };

        for file in files.flatten() {
            let Some(name) = file.file_name().to_str().map(str::to_owned) else {
                continue;
            };

            let is_match = name.len() == 12
                && name[..3].eq_ignore_ascii_case(mech_type)
                && name[3..5].bytes().all(|b| b.is_ascii_digit())
                && name[5..].eq_ignore_ascii_case("usr.mek");
            if !is_match {
                continue;
            }

            let n: usize = name[3..5].parse().expect("two ASCII digits");
            let i = 100 + n;
            if let Some(slot) = filenames.get_mut(i) {
                write_slot(slot.as_mut_ptr(), &name.as_bytes()[..8]);
            }
        }
    }
}

patches!(
    pub(in crate::shell) static PATCHES = [
        hook mechbay,
        hook load_mech_variant_list,
    ];
);
