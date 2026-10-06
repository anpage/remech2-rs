//! New egui-based controls config UI that replaces the Cockpit Controls screen.

mod capture;
mod editor;
mod labels;

use std::{
    collections::{BTreeMap, HashMap},
    sync::atomic::{AtomicBool, Ordering},
    time::Instant,
};

use egui::{
    Align, Align2, Color32, ComboBox, Context, DragValue, FontFamily, FontId, FontSelection, Id,
    Layout, Modal, Pos2, ProgressBar, Rect, RichText, ScrollArea, Sense, Shape, Slider, Stroke,
    StrokeKind, TextEdit, TextStyle, Ui, pos2, text::LayoutJob, vec2,
};

use capture::{Capture, Wanted};
use editor::Editor;

use super::{
    KeyCode,
    action::{ActionId, ActionKind, AxisAction, AxisBehaviour, Category},
    binding::{AxisMode, AxisTuning, Binding, Direction},
    defaults,
    kbm::{self, KbmState},
    pad::{self, BackendStatus, Device, HatDirection},
    source::{MouseAxis, Source, SourceKind, Threshold},
    store::{self, Store},
};

static OPEN_REQUESTED: AtomicBool = AtomicBool::new(false);
static HAS_KEYBOARD: AtomicBool = AtomicBool::new(false);
static CAPTURING: AtomicBool = AtomicBool::new(false);

/// Toggled by Ctrl+Shift+D
static SHOW_DEBUG: AtomicBool = AtomicBool::new(false);

pub fn request_open() {
    OPEN_REQUESTED.store(true, Ordering::Relaxed);
}

pub fn has_keyboard() -> bool {
    HAS_KEYBOARD.load(Ordering::Relaxed)
}

pub fn capturing() -> bool {
    CAPTURING.load(Ordering::Relaxed)
}

const WIDTH: f32 = 610.0;
const HEIGHT: f32 = 450.0;

const MIN_LIST_HEIGHT: f32 = 120.0;

const PREFIX_GAP: f32 = 3.5;
const INPUT_GAP: f32 = 3.0;

#[derive(Clone, Copy, PartialEq, Eq)]
enum Tab {
    Bindings,
    Devices,
}

struct Capturing {
    action: ActionId,
    direction: Direction,
    replace: Option<usize>,
    capture: Capture,
}

struct Identifying {
    slot: String,
    before: Vec<Device>,
}

#[derive(Clone, PartialEq)]
enum NameFor {
    NewProfile,
    CopyProfile,
    RenameProfile,
    RenameSlot(String),
}

struct NamePrompt {
    purpose: NameFor,
    text: String,
    error: Option<String>,
    focused: bool,
}

impl NamePrompt {
    fn new(purpose: NameFor, text: String) -> Self {
        Self {
            purpose,
            text,
            error: None,
            focused: false,
        }
    }
}

enum Confirm {
    DeleteProfile,
    RemoveSlot(String),
    Close,
}

pub struct ControlsWindow {
    editor: Option<Editor>,
    tab: Tab,
    selected: Option<usize>,
    capture: Option<Capturing>,
    identify: Option<Identifying>,
    prompt: Option<NamePrompt>,
    confirm: Option<Confirm>,
    profiles: Vec<String>,
    prompt_rect: Option<Rect>,
    debug_key_presses: u32,
    last_frame: Instant,
}

impl Default for ControlsWindow {
    fn default() -> Self {
        Self {
            editor: None,
            tab: Tab::Bindings,
            selected: None,
            capture: None,
            identify: None,
            prompt: None,
            confirm: None,
            profiles: Vec::new(),
            prompt_rect: None,
            debug_key_presses: 0,
            last_frame: Instant::now(),
        }
    }
}

pub const FONT_FAMILY: &str = "controls";

fn scale_style(ui: &mut Ui, scale: f32) {
    let style = ui.style_mut();
    style.text_styles = text_styles(scale);
    let spacing = &mut style.spacing;
    spacing.item_spacing = vec2(4.0, 3.0) * scale;
    spacing.button_padding = vec2(3.0, 1.0) * scale;
    spacing.interact_size = vec2(20.0, 11.0) * scale;
    spacing.slider_width = 80.0 * scale;
    spacing.icon_width = 8.0 * scale;
    spacing.icon_width_inner = 5.0 * scale;
    spacing.icon_spacing = 3.0 * scale;
    spacing.indent = 8.0 * scale;
    spacing.combo_width = 90.0 * scale;
}

fn text_styles(scale: f32) -> BTreeMap<TextStyle, FontId> {
    let proportional = |size: f32| FontId::new(size * scale, FontFamily::Name(FONT_FAMILY.into()));
    [
        (TextStyle::Small, proportional(6.0)),
        (TextStyle::Body, proportional(7.5)),
        (TextStyle::Button, proportional(7.5)),
        (TextStyle::Heading, proportional(10.0)),
        (TextStyle::Monospace, FontId::monospace(7.0 * scale)),
    ]
    .into()
}

fn slot_namer(editor: &Editor) -> impl Fn(&str) -> String + '_ {
    |id: &str| {
        editor
            .profile()
            .slot(id)
            .map_or_else(|| id.to_owned(), |slot| slot.name.clone())
    }
}

fn slot_device<'a>(editor: &Editor, devices: &'a [Device], slot: &str) -> Option<&'a Device> {
    let id = editor.assignment().device(slot)?;
    devices
        .iter()
        .find(|device| device.id == id && device.connected)
}

impl ControlsWindow {
    pub fn is_open(&self) -> bool {
        self.editor.is_some()
    }

    fn open(&mut self) {
        let editor = Editor::open(Store::default(), &store::active_profile_name());
        self.profiles = editor.store().profiles();
        self.editor = Some(editor);
        self.selected = None;
        self.last_frame = Instant::now();
    }

    fn close(&mut self) {
        self.capture = None;
        self.identify = None;
        self.prompt = None;
        self.confirm = None;
        self.selected = None;
    }

    pub fn window(&mut self, ctx: &Context, scale: f32) {
        if OPEN_REQUESTED.swap(false, Ordering::Relaxed) && self.editor.is_none() {
            self.open();
        }

        let Some(mut editor) = self.editor.take() else {
            HAS_KEYBOARD.store(false, Ordering::Relaxed);
            CAPTURING.store(false, Ordering::Relaxed);
            return;
        };

        let now = Instant::now();
        let seconds = now.duration_since(self.last_frame).as_secs_f64();
        self.last_frame = now;
        let kbm = kbm::snapshot();
        let pads = pad::snapshot();
        editor.match_devices(&pads.devices);
        editor.evaluate(&kbm, &pads.devices, seconds);

        self.check_debug_hotkey(&kbm);
        self.update_capture(ctx, &mut editor, &kbm, &pads.devices);
        self.update_identify(&mut editor, &pads.devices);

        if (self.prompt.is_some() || self.confirm.is_some())
            && ctx.input_mut(|input| input.consume_key(egui::Modifiers::NONE, egui::Key::Escape))
        {
            self.prompt = None;
            self.confirm = None;
        }

        let outside = ctx.style();
        ctx.style_mut(|style| style.text_styles = text_styles(scale));

        let mut close = false;
        let response = Modal::new(Id::new("controls_window")).show(ctx, |ui| {
            scale_style(ui, scale);
            ui.set_width(WIDTH * scale);
            ui.set_min_height(HEIGHT * scale);
            close = self.contents(ui, &mut editor, &pads, scale);
        });
        let busy = self.capture.is_some() || self.identify.is_some();
        if !busy && response.should_close() && !response.backdrop_response.clicked() {
            if editor.has_changes() {
                self.prompt = None;
                self.confirm = Some(Confirm::Close);
            } else {
                close = true;
            }
        }

        self.capture_prompt(ctx, &mut editor, scale);
        self.identify_prompt(ctx, &editor, scale);
        ctx.set_style(outside);

        if close {
            self.close();
        } else {
            self.editor = Some(editor);
        }
        HAS_KEYBOARD.store(self.editor.is_some(), Ordering::Relaxed);
        CAPTURING.store(self.capture.is_some(), Ordering::Relaxed);
    }

    fn check_debug_hotkey(&mut self, kbm: &KbmState) {
        let presses = kbm.key_presses(KeyCode::KeyD);
        let held = |keys: [KeyCode; 2]| keys.iter().any(|key| kbm.keys_held.contains(key));
        if presses != self.debug_key_presses
            && self.capture.is_none()
            && held([KeyCode::ControlLeft, KeyCode::ControlRight])
            && held([KeyCode::ShiftLeft, KeyCode::ShiftRight])
        {
            SHOW_DEBUG.fetch_xor(true, Ordering::Relaxed);
        }
        self.debug_key_presses = presses;
    }

    fn start_capture(&mut self, action: ActionId, replace: Option<usize>, direction: Direction) {
        let kbm = kbm::snapshot();
        let pads = pad::snapshot();
        self.capture = Some(Capturing {
            action,
            direction,
            replace,
            capture: Capture::new(&kbm, &pads.devices),
        });
        self.prompt_rect = None;
    }

    fn update_capture(
        &mut self,
        ctx: &Context,
        editor: &mut Editor,
        kbm: &KbmState,
        devices: &[Device],
    ) {
        let Some(capturing) = &mut self.capture else {
            return;
        };
        let over_prompt = self
            .prompt_rect
            .zip(ctx.pointer_latest_pos())
            .is_some_and(|(rect, pointer)| rect.contains(pointer));
        let wanted = Wanted {
            axes: capturing.action.action().axis().is_some(),
            ignore_mouse: over_prompt,
        };
        if let Some(captured) = capturing.capture.update(kbm, devices, wanted) {
            let capturing = self.capture.take().unwrap();
            let action = capturing.action.action().name;
            if let Some(binding) = editor.binding_for(action, &captured, devices) {
                self.place(editor, &capturing, binding);
            }
        }
    }

    fn place(&mut self, editor: &mut Editor, capturing: &Capturing, mut binding: Binding) {
        if capturing.action.action().axis().is_some()
            && binding.source.kind() == SourceKind::Digital
        {
            binding.direction = Some(capturing.direction);
        }
        self.selected = Some(match capturing.replace {
            Some(index) => {
                editor.edit(|profile| {
                    if let Some(old) = profile.bindings.get_mut(index) {
                        old.source = binding.source;
                        old.modifiers = binding.modifiers;
                        old.direction = old.direction.or(binding.direction);
                    }
                });
                index
            }
            None => editor.add_binding(binding),
        });
    }

    fn capture_prompt(&mut self, ctx: &Context, editor: &mut Editor, scale: f32) {
        let Some(capturing) = &mut self.capture else {
            return;
        };
        let action = capturing.action.action();
        let axis = action.axis();
        let is_axis = axis.is_some();
        let mut chosen = None;
        let mut cancel = false;
        let response = Modal::new(Id::new("controls_capture")).show(ctx, |ui| {
            scale_style(ui, scale);
            ui.set_width(220.0 * scale);
            ui.vertical_centered(|ui| {
                ui.heading(action.label);
                ui.add_space(12.0);
                ui.label("Press a button or move an axis.");
                ui.add_space(12.0);
            });
            if let Some(axis) = axis {
                direction_picker(ui, axis, &mut capturing.direction, "Direction");
            }
            ui.horizontal(|ui| {
                if is_axis {
                    if ui.button("Mouse X").clicked() {
                        chosen = Some(Source::MouseAxis(MouseAxis::X));
                    }
                    if ui.button("Mouse Y").clicked() {
                        chosen = Some(Source::MouseAxis(MouseAxis::Y));
                    }
                }
                ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                    cancel = ui.button("Cancel").clicked();
                });
            });
        });
        self.prompt_rect = Some(response.response.rect);

        if cancel {
            self.capture = None;
        } else if let Some(source) = chosen {
            let capturing = self.capture.take().unwrap();
            let binding = Binding::new(capturing.action.action().name, source);
            self.place(editor, &capturing, binding);
        }
    }

    fn update_identify(&mut self, editor: &mut Editor, devices: &[Device]) {
        let Some(identify) = &mut self.identify else {
            return;
        };
        match capture::device_pressed(&identify.before, devices) {
            Some(id) => {
                let slot = identify.slot.clone();
                if let Some(device) = devices.iter().find(|device| device.id == id) {
                    editor.claim(&slot, device);
                }
                self.identify = None;
            }
            None => identify.before = devices.to_vec(),
        }
    }

    fn identify_prompt(&mut self, ctx: &Context, editor: &Editor, scale: f32) {
        let Some(identify) = &self.identify else {
            return;
        };
        let name = slot_namer(editor)(&identify.slot);
        let mut cancel = false;
        let response = Modal::new(Id::new("controls_identify")).show(ctx, |ui| {
            scale_style(ui, scale);
            ui.set_width(200.0 * scale);
            ui.vertical_centered(|ui| {
                ui.heading(format!("Which is {name}?"));
                ui.label("Press any button on it.");
                cancel = ui.button("Cancel").clicked();
            });
        });
        if cancel || response.should_close() {
            self.identify = None;
        }
    }

    fn contents(
        &mut self,
        ui: &mut Ui,
        editor: &mut Editor,
        pads: &pad::PadState,
        scale: f32,
    ) -> bool {
        let top = ui.cursor().top();
        let close = self.header(ui, editor, scale);
        ui.separator();

        ui.horizontal(|ui| {
            ui.selectable_value(&mut self.tab, Tab::Bindings, "Bindings");
            ui.selectable_value(&mut self.tab, Tab::Devices, "Controllers");
        });
        ui.separator();

        let used = ui.cursor().top() - top;
        let list_height = (HEIGHT * scale - used).max(MIN_LIST_HEIGHT * scale);
        match self.tab {
            Tab::Bindings => self.bindings_tab(ui, editor, &pads.devices, list_height, scale),
            Tab::Devices => self.devices_tab(ui, editor, pads, list_height, scale),
        }
        close
    }

    fn header(&mut self, ui: &mut Ui, editor: &mut Editor, scale: f32) -> bool {
        let mut close = false;
        let mut switch_to = None;
        let edited = editor.edited();
        let unapplied = "Apply or discard your changes first";
        ui.horizontal(|ui| {
            ui.label(RichText::new("COCKPIT CONTROLS").font(FontId::proportional(12.0 * scale)));
            ui.add_space(8.0 * scale);
            ui.label("Profile");
            ui.add_enabled_ui(!edited, |ui| {
                ComboBox::from_id_salt("controls_profile")
                    .selected_text(editor.name())
                    .width(110.0 * scale)
                    .show_ui(ui, |ui| {
                        scale_style(ui, scale);
                        for name in &self.profiles {
                            if ui.selectable_label(name == editor.name(), name).clicked() {
                                switch_to = Some(name.clone());
                            }
                        }
                    })
                    .response
                    .on_disabled_hover_text(unapplied);
            });
            let mut ask_name = |purpose: NameFor, text: String| {
                self.prompt = Some(NamePrompt::new(purpose, text));
                self.confirm = None;
            };
            if ui
                .add_enabled(!edited, egui::Button::new("New"))
                .on_hover_text("Make a new profile with the original controls")
                .on_disabled_hover_text(unapplied)
                .clicked()
            {
                ask_name(NameFor::NewProfile, String::new());
            }
            if ui
                .add_enabled(!edited, egui::Button::new("Copy"))
                .on_hover_text("Make a copy of this profile")
                .on_disabled_hover_text(unapplied)
                .clicked()
            {
                ask_name(NameFor::CopyProfile, format!("{} copy", editor.name()));
            }
            if ui
                .add_enabled(!editor.is_default(), egui::Button::new("Rename"))
                .on_disabled_hover_text("The default controls can't be renamed")
                .clicked()
            {
                ask_name(NameFor::RenameProfile, editor.name().to_owned());
            }
            let delete =
                ui.add_enabled(!editor.is_default() && !edited, egui::Button::new("Delete"));
            let delete = if editor.is_default() {
                delete.on_disabled_hover_text("The default controls can't be deleted")
            } else {
                delete.on_disabled_hover_text(unapplied)
            };
            if delete.clicked() {
                self.prompt = None;
                self.confirm = Some(Confirm::DeleteProfile);
            }

            ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                if ui.button("Close").clicked() {
                    if editor.has_changes() {
                        self.prompt = None;
                        self.confirm = Some(Confirm::Close);
                    } else {
                        close = true;
                    }
                }
                if ui
                    .add_enabled(editor.has_changes(), egui::Button::new("Apply"))
                    .on_hover_text("Save these controls")
                    .clicked()
                {
                    apply(editor);
                }
            });
        });

        if let Some(name) = switch_to {
            editor.switch(&name);
            self.selected = None;
        }
        self.name_prompt(ui, editor, scale);
        close |= self.confirm_prompt(ui, editor);
        if let Some(error) = &editor.save_error {
            ui.colored_label(ui.visuals().error_fg_color, format!("Not saved: {error}"));
        }
        close
    }

    fn name_prompt(&mut self, ui: &mut Ui, editor: &mut Editor, scale: f32) {
        let Some(prompt) = &mut self.prompt else {
            return;
        };
        let mut done = false;
        let mut cancel = false;
        ui.horizontal(|ui| {
            ui.label(match prompt.purpose {
                NameFor::RenameSlot(_) => "Controller name",
                _ => "Profile name",
            });
            let field = ui.add(TextEdit::singleline(&mut prompt.text).desired_width(150.0 * scale));

            if !prompt.focused {
                field.request_focus();
                prompt.focused = true;
            }

            let entered =
                field.lost_focus() && ui.input(|input| input.key_pressed(egui::Key::Enter));
            done = ui.button("OK").clicked() || entered;
            cancel = ui.button("Cancel").clicked();
        });
        if let Some(error) = &prompt.error {
            ui.colored_label(ui.visuals().error_fg_color, error);
        }
        if cancel {
            self.prompt = None;
            return;
        }
        if !done {
            return;
        }

        let text = prompt.text.trim().to_owned();
        let active = editor.active().to_owned();
        let result = match &prompt.purpose {
            NameFor::RenameSlot(slot) => {
                if text.is_empty() {
                    Err(anyhow::anyhow!("The name can't be empty"))
                } else {
                    editor.edit(|profile| {
                        if let Some(slot) = profile.devices.iter_mut().find(|s| &s.id == slot) {
                            slot.name = text;
                        }
                    });
                    Ok(())
                }
            }
            _ if !store::valid_name(&text) => Err(anyhow::anyhow!(
                "Names can't be empty, start or end with a dot, or use \\ / : * ? \" < > |"
            )),
            NameFor::NewProfile => editor.create(&text, &defaults::classic()),
            NameFor::CopyProfile => {
                let profile = editor.profile().clone();
                editor.create(&text, &profile)
            }
            NameFor::RenameProfile => editor.rename(&text),
        };
        match result {
            Ok(()) => {
                if !matches!(prompt.purpose, NameFor::RenameSlot(_)) {
                    if editor.active() != active {
                        store::set_active_profile_name(editor.active());
                    }
                    self.profiles = editor.store().profiles();
                    self.selected = None;
                }
                self.prompt = None;
            }
            Err(e) => prompt.error = Some(format!("{e:#}")),
        }
    }

    fn confirm_prompt(&mut self, ui: &mut Ui, editor: &mut Editor) -> bool {
        let Some(confirm) = &self.confirm else {
            return false;
        };
        let question = match confirm {
            Confirm::DeleteProfile => format!("Delete the profile {:?}?", editor.name()),
            Confirm::RemoveSlot(slot) => format!(
                "Remove {} and its {} bindings?",
                slot_namer(editor)(slot),
                editor.bindings_on(slot)
            ),
            Confirm::Close => return self.close_prompt(ui, editor),
        };
        let mut answer = None;
        ui.horizontal(|ui| {
            ui.label(question);
            if ui.button("Yes").clicked() {
                answer = Some(true);
            }
            if ui.button("No").clicked() {
                answer = Some(false);
            }
        });
        let Some(answer) = answer else {
            return false;
        };
        let confirm = self.confirm.take().unwrap();
        if !answer {
            return false;
        }
        match confirm {
            Confirm::DeleteProfile => {
                let active = editor.active().to_owned();
                match editor.delete() {
                    Ok(()) => {
                        if editor.active() != active {
                            store::set_active_profile_name(editor.active());
                        }
                        self.profiles = editor.store().profiles();
                        self.selected = None;
                    }
                    Err(e) => editor.save_error = Some(format!("{e:#}")),
                }
            }
            Confirm::RemoveSlot(slot) => {
                editor.remove_slot(&slot);
                self.selected = None;
            }
            Confirm::Close => {}
        }
        false
    }

    fn close_prompt(&mut self, ui: &mut Ui, editor: &mut Editor) -> bool {
        let mut close = false;
        ui.horizontal(|ui| {
            ui.label("Apply your changes before closing?");
            if ui.button("Apply").clicked() {
                close = apply(editor);
            }
            if ui
                .button("Discard")
                .on_hover_text("Close and discard changes")
                .clicked()
            {
                close = true;
            }
            if ui.button("Keep editing").clicked() {
                self.confirm = None;
            }
        });
        if close {
            self.confirm = None;
        }
        close
    }

    fn bindings_tab(
        &mut self,
        ui: &mut Ui,
        editor: &mut Editor,
        devices: &[Device],
        height: f32,
        scale: f32,
    ) {
        ui.horizontal_top(|ui| {
            ui.vertical(|ui| {
                ui.set_width(360.0 * scale);
                ScrollArea::vertical()
                    .id_salt("controls_actions")
                    .min_scrolled_height(height)
                    .max_height(height)
                    .auto_shrink([false, false])
                    .show(ui, |ui| self.action_list(ui, editor, scale));
            });
            ui.separator();
            ui.vertical(|ui| {
                ScrollArea::vertical()
                    .id_salt("controls_binding")
                    .min_scrolled_height(height)
                    .max_height(height)
                    .auto_shrink([false, false])
                    .show(ui, |ui| self.binding_editor(ui, editor, devices, scale));
            });
        });
    }

    fn action_list(&mut self, ui: &mut Ui, editor: &Editor, scale: f32) {
        let show_debug = SHOW_DEBUG.load(Ordering::Relaxed);
        let mut by_category: BTreeMap<Category, Vec<ActionId>> = BTreeMap::new();
        for id in ActionId::all() {
            let category = id.action().category;
            if category != Category::Debug || show_debug {
                by_category.entry(category).or_default().push(id);
            }
        }

        let mut by_action: HashMap<&str, Vec<usize>> = HashMap::new();
        let mut unknown = Vec::new();
        for (index, binding) in editor.profile().bindings.iter().enumerate() {
            match ActionId::find(&binding.action) {
                Some(id) => by_action.entry(id.action().name).or_default().push(index),
                None => unknown.push(index),
            }
        }

        for (category, ids) in by_category {
            egui::CollapsingHeader::new(RichText::new(category.label()).strong().heading())
                .id_salt(("controls_category", category))
                .default_open(true)
                .show(ui, |ui| {
                    for id in ids {
                        let bindings = by_action
                            .get(id.action().name)
                            .map_or(&[][..], Vec::as_slice);
                        self.action_row(ui, editor, id, bindings, scale);
                    }
                });
        }

        if !unknown.is_empty() {
            egui::CollapsingHeader::new(RichText::new("Unknown actions").strong())
                .id_salt("controls_unknown")
                .default_open(true)
                .show(ui, |ui| {
                    ui.horizontal_wrapped(|ui| {
                        for index in unknown {
                            self.chip(ui, editor, index, scale);
                        }
                    });
                });
        }
    }

    fn action_row(
        &mut self,
        ui: &mut Ui,
        editor: &Editor,
        id: ActionId,
        bindings: &[usize],
        scale: f32,
    ) {
        let action = id.action();
        ui.horizontal(|ui| {
            indicator(ui, editor, id, scale);
            let label_width = 85.0 * scale;
            ui.allocate_ui_with_layout(
                vec2(label_width, 11.0 * scale),
                Layout::left_to_right(Align::Center),
                |ui| {
                    ui.set_min_width(label_width);
                    ui.add(egui::Label::new(RichText::new(action.label).strong()).truncate());
                },
            );
            let add = ui.add(egui::Button::new("").min_size(vec2(11.0, 11.0) * scale));
            let stroke = ui.style().interact(&add).fg_stroke;
            let (center, arm) = (add.rect.center(), 2.5 * scale);
            ui.painter()
                .hline(center.x - arm..=center.x + arm, center.y, stroke);
            ui.painter()
                .vline(center.x, center.y - arm..=center.y + arm, stroke);
            if add.on_hover_text("Add a binding").clicked() {
                self.start_capture(id, None, Direction::Increase);
            }
            ui.horizontal_wrapped(|ui| {
                for &index in bindings {
                    self.chip(ui, editor, index, scale);
                }
            });
        });
    }

    fn chip(&mut self, ui: &mut Ui, editor: &Editor, index: usize, scale: f32) {
        let binding = &editor.profile().bindings[index];
        let axis = ActionId::find(&binding.action).and_then(|id| id.action().axis());
        let prefix = match (axis, binding.set_to, binding.direction) {
            (Some(_), Some(set_to), _) => Some(percent(set_to)),
            (Some(axis), None, Some(direction)) => {
                Some(direction_label(axis, direction).to_owned())
            }
            _ => None,
        };

        let problem = editor.problem(index);
        let shared = editor.shared_with(index);
        let color = if problem.is_some() {
            Some(ui.visuals().error_fg_color)
        } else if !shared.is_empty() {
            Some(ui.visuals().warn_fg_color)
        } else {
            None
        };

        let mut label = LayoutJob::default();
        let append = |label: &mut LayoutJob, mut part: RichText, color: Option<Color32>| {
            if let Some(color) = color {
                part = part.color(color);
            }
            part.append_to(label, ui.style(), FontSelection::Default, Align::Center);
        };

        let mut divider = None;
        if let Some(prefix) = prefix {
            append(&mut label, RichText::new(prefix).weak(), None);
            let width = ui.fonts_mut(|fonts| fonts.layout_job(label.clone()).size().x);
            divider = Some(width + PREFIX_GAP * scale);
        }
        let input = label.sections.len();
        append(
            &mut label,
            RichText::new(labels::chord(binding, &slot_namer(editor))).monospace(),
            color,
        );
        if divider.is_some() {
            label.sections[input].leading_space = (PREFIX_GAP + INPUT_GAP) * scale;
        }
        let selected = self.selected == Some(index);
        let mut response = ui.selectable_label(selected, label);

        if let Some(divider) = divider {
            let x = response.rect.left() + ui.spacing().button_padding.x + divider;
            ui.painter().vline(
                x,
                response.rect.y_range(),
                ui.visuals().widgets.noninteractive.bg_stroke,
            );
        }

        if !selected && !response.hovered() {
            let visuals = ui.visuals();
            ui.painter().rect_stroke(
                response.rect,
                visuals.widgets.inactive.corner_radius,
                visuals.widgets.noninteractive.bg_stroke,
                StrokeKind::Inside,
            );
        }

        if let Some(problem) = problem {
            response = response.on_hover_ui(|ui| {
                ui.label(format!("Not used: {problem}"));
            });
        } else if !shared.is_empty() {
            response = response.on_hover_ui(|ui| {
                ui.label(format!("Also drives {}", shared.join(", ")));
            });
        }
        if response.clicked() {
            self.selected = Some(index);
        }
    }

    fn binding_editor(&mut self, ui: &mut Ui, editor: &mut Editor, devices: &[Device], scale: f32) {
        let Some(index) = self
            .selected
            .filter(|&index| index < editor.profile().bindings.len())
        else {
            ui.label("Pick a binding or add a new one.");
            return;
        };
        let original = editor.profile().bindings[index].clone();
        let mut binding = original.clone();
        let id = ActionId::find(&binding.action);
        let action = id.map(ActionId::action);

        ui.heading(action.map_or(binding.action.as_str(), |action| action.label));
        ui.add_space(8.0);
        ui.label(labels::chord(&binding, &slot_namer(editor)));
        ui.add_space(8.0);
        if let Some(problem) = editor.problem(index) {
            ui.colored_label(ui.visuals().error_fg_color, format!("Not used: {problem}"));
            ui.add_space(8.0);
        }
        let shared = editor.shared_with(index);
        if !shared.is_empty() {
            ui.colored_label(
                ui.visuals().warn_fg_color,
                format!("This input also drives {}", shared.join(", ")),
            );
            ui.add_space(8.0);
        }

        ui.horizontal(|ui| {
            if let Some(id) = id
                && ui
                    .button("Rebind")
                    .on_hover_text("Choose a new input, keeping the settings")
                    .clicked()
            {
                let direction = binding.direction.unwrap_or(Direction::Increase);
                self.start_capture(id, Some(index), direction);
            }
            if ui.button("Remove").clicked() {
                editor.remove_binding(index);
                self.selected = None;
            }
        });
        if self.selected.is_none() {
            return;
        }

        ui.separator();

        if let Source::AxisThreshold { threshold, .. } = &mut binding.source {
            threshold_editor(ui, threshold);
        }

        if let Some(action) = action {
            match (action.kind, binding.source.kind()) {
                (ActionKind::Hold, SourceKind::Digital) => {
                    ui.checkbox(&mut binding.toggle, "Each press turns it on or off");
                }
                (ActionKind::Axis(axis), SourceKind::Digital) => {
                    button_on_axis_editor(ui, &axis, &mut binding);
                }
                (ActionKind::Axis(axis), SourceKind::Analog) => {
                    let relative_allowed =
                        axis.behaviour == AxisBehaviour::Position { relative: true };
                    let reading = axis_reading(editor, devices, &binding.source);
                    axis_editor(ui, &mut binding.tuning, relative_allowed, reading, scale);
                }
                (ActionKind::Axis(_), SourceKind::Delta) => {
                    ui.checkbox(&mut binding.tuning.invert, "Invert");
                    ui.add(
                        Slider::new(&mut binding.tuning.scale, 0.1..=5.0)
                            .logarithmic(true)
                            .text("Sensitivity"),
                    );
                }
                _ => {}
            }
        }

        if binding != original {
            editor.edit(|profile| profile.bindings[index] = binding);
        }
    }

    fn devices_tab(
        &mut self,
        ui: &mut Ui,
        editor: &mut Editor,
        pads: &pad::PadState,
        height: f32,
        scale: f32,
    ) {
        let devices = &pads.devices;
        let text = |text: String| RichText::new(text).small();
        ScrollArea::vertical()
            .id_salt("controls_devices")
            .min_scrolled_height(height)
            .max_height(height)
            .auto_shrink([false, false])
            .show(ui, |ui| {
                ui.label(RichText::new("Controllers").strong());
                match &pads.status {
                    BackendStatus::Starting => {
                        ui.label("Looking for controllers...");
                    }
                    BackendStatus::Failed(e) => {
                        ui.colored_label(
                            ui.visuals().error_fg_color,
                            format!("Controllers can't be used: {e}"),
                        );
                    }
                    BackendStatus::Running => {}
                }

                for slot in editor.profile().devices.clone() {
                    let device = slot_device(editor, devices, &slot.id);
                    egui::CollapsingHeader::new(&slot.name)
                        .id_salt(("controls_slot", &slot.id))
                        .default_open(device.is_some())
                        .show(ui, |ui| {
                            let status = match device {
                                Some(device) => format!(
                                    "Using {}, with {} bindings",
                                    device.name,
                                    editor.bindings_on(&slot.id)
                                ),
                                None if devices.iter().any(|device| {
                                    device.connected
                                        && editor.assignment().slot(device.id).is_none()
                                        && profile_usb_matches(&slot, device)
                                }) =>
                                {
                                    "Not sure which it is: use Identify, then press a button on it"
                                        .to_owned()
                                }
                                None => format!("Not connected ({})", slot.product),
                            };
                            ui.label(status);
                            ui.add_space(8.0);
                            ui.horizontal(|ui| {
                                if ui.button("Rename").clicked() {
                                    self.confirm = None;
                                    self.prompt = Some(NamePrompt::new(
                                        NameFor::RenameSlot(slot.id.clone()),
                                        slot.name.clone(),
                                    ));
                                }
                                if ui
                                    .button("Identify")
                                    .on_hover_text(
                                        "Pick which controller this is by pressing a button on it",
                                    )
                                    .clicked()
                                {
                                    self.identify = Some(Identifying {
                                        slot: slot.id.clone(),
                                        before: devices.clone(),
                                    });
                                }
                                if ui.button("Remove").clicked() {
                                    self.prompt = None;
                                    self.confirm = Some(Confirm::RemoveSlot(slot.id.clone()));
                                }
                            });
                            if let Some(device) = device {
                                device_controls(ui, device, scale, &text);
                            }
                        });
                }

                let others: Vec<&Device> = devices
                    .iter()
                    .filter(|device| {
                        device.connected && editor.assignment().slot(device.id).is_none()
                    })
                    .collect();
                if !others.is_empty() {
                    ui.separator();
                    ui.label(RichText::new("Not in this profile").strong());
                    for device in others {
                        ui.horizontal(|ui| {
                            ui.label(&device.name);
                            if ui
                                .button("Add")
                                .on_hover_text("Binding one of its controls adds it too")
                                .clicked()
                            {
                                editor.slot_for(device);
                            }
                        });
                    }
                }
                if devices.iter().all(|device| !device.connected)
                    && editor.profile().devices.is_empty()
                {
                    ui.label("No controllers are connected.");
                }
            });
    }
}

fn apply(editor: &mut Editor) -> bool {
    let applied = editor.apply().is_ok();
    if applied {
        store::set_active_profile_name(editor.active());
    }
    applied
}

fn profile_usb_matches(slot: &crate::input::profile::DeviceSlot, device: &Device) -> bool {
    slot.usb_id.is_some() && slot.usb_id == crate::input::profile::UsbId::of(device)
}

fn indicator(ui: &mut Ui, editor: &Editor, id: ActionId, scale: f32) {
    let (rect, _) = ui.allocate_exact_size(vec2(14.0, 8.0) * scale, Sense::hover());
    if !ui.is_rect_visible(rect) {
        return;
    }
    let visuals = ui.visuals();
    let output = editor.state()[id];
    let painter = ui.painter();
    let on = visuals.selection.bg_fill;
    let off = visuals.widgets.inactive.bg_fill;
    match id.action().kind {
        ActionKind::Axis(axis) => {
            painter.rect_filled(rect, 1.0, off);
            let one_sided = axis.min == 0.0;
            let (start, span) = if one_sided {
                (rect.left(), rect.width())
            } else {
                (rect.center().x, rect.width() / 2.0)
            };
            let end = start + output.value as f32 * span;
            let bar = Rect::from_x_y_ranges(start.min(end)..=start.max(end), rect.y_range());
            painter.rect_filled(bar, 0.0, on);
            if !one_sided {
                painter.line_segment(
                    [pos2(start, rect.top()), pos2(start, rect.bottom())],
                    Stroke::new(1.0_f32, visuals.weak_text_color()),
                );
            }
        }
        _ => {
            let lit = output.held || output.presses > 0;
            painter.circle_filled(rect.center(), 2.5 * scale, if lit { on } else { off });
        }
    }
}

fn percent(value: f64) -> String {
    format!("{:.0}%", value * 100.0)
}

fn direction_label(axis: &AxisAction, direction: Direction) -> &'static str {
    match direction {
        Direction::Increase => axis.directions[0],
        Direction::Decrease => axis.directions[1],
    }
}

fn direction_picker(ui: &mut Ui, axis: &AxisAction, direction: &mut Direction, label: &str) {
    ui.horizontal(|ui| {
        ui.label(label);
        for way in [Direction::Decrease, Direction::Increase] {
            ui.radio_value(direction, way, direction_label(axis, way));
        }
    });
}

fn threshold_editor(ui: &mut Ui, threshold: &mut Threshold) {
    let (mut above, mut value) = match *threshold {
        Threshold::Above(value) => (true, value),
        Threshold::Below(value) => (false, value),
    };
    ui.horizontal(|ui| {
        ui.label("On when the axis is");
        ui.radio_value(&mut above, true, "above");
        ui.radio_value(&mut above, false, "below");
        ui.add(
            DragValue::new(&mut value)
                .range(-1.0..=1.0)
                .speed(0.01)
                .fixed_decimals(2),
        );
    });
    *threshold = if above {
        Threshold::Above(value)
    } else {
        Threshold::Below(value)
    };
}

fn button_on_axis_editor(ui: &mut Ui, axis: &AxisAction, binding: &mut Binding) {
    // One that holds its position can be put somewhere instead of pushed
    if matches!(axis.behaviour, AxisBehaviour::Position { .. }) {
        let mut sets = binding.set_to.is_some();
        ui.checkbox(&mut sets, "Sets it to a position")
            .on_hover_text("Pressing the button sets the axis to a specific position");
        match (sets, binding.set_to) {
            (true, None) => binding.set_to = Some((axis.min + 1.0) / 2.0),
            (false, Some(_)) => binding.set_to = None,
            _ => {}
        }
        if let Some(set_to) = binding.set_to {
            let mut percent = set_to * 100.0;
            if ui
                .add(
                    Slider::new(&mut percent, axis.min * 100.0..=100.0)
                        .step_by(1.0)
                        .suffix("%")
                        .fixed_decimals(0)
                        .text("Position"),
                )
                .changed()
            {
                binding.set_to = Some(percent / 100.0);
            }
            return;
        }
    }

    let mut direction = binding.direction.unwrap_or(Direction::Increase);
    direction_picker(ui, axis, &mut direction, "Direction");
    binding.direction = Some(direction);

    let wheel = matches!(binding.source, Source::Wheel(_));
    let mut steps = binding.step.is_some();
    ui.checkbox(&mut steps, "Moves a set amount each press")
        .on_hover_text("Otherwise it speeds up while held");
    match (steps, binding.step) {
        (true, None) => binding.step = Some(0.1),
        (false, Some(_)) => binding.step = None,
        _ => {}
    }
    if let Some(step) = &mut binding.step {
        ui.add(Slider::new(step, 0.01..=1.0).text("Step"));
    } else if wheel {
        ui.add(
            Slider::new(&mut binding.tuning.scale, 0.1..=5.0)
                .logarithmic(true)
                .text("Step size"),
        );
    } else {
        ui.add(
            Slider::new(&mut binding.tuning.scale, 0.1..=5.0)
                .logarithmic(true)
                .text("Speed"),
        );
    }
}

fn axis_editor(
    ui: &mut Ui,
    tuning: &mut AxisTuning,
    relative_allowed: bool,
    reading: Option<f64>,
    scale: f32,
) {
    if relative_allowed {
        ui.horizontal(|ui| {
            ui.radio_value(&mut tuning.mode, AxisMode::Absolute, "Aboslute")
                .on_hover_text("The axis follows the stick's position");
            ui.radio_value(&mut tuning.mode, AxisMode::Relative, "Relative")
                .on_hover_text("The stick pushes the axis");
        });
    } else {
        tuning.mode = AxisMode::Absolute;
    }
    let relative = tuning.mode == AxisMode::Relative;

    ui.checkbox(&mut tuning.invert, "Invert");
    ui.add(Slider::new(&mut tuning.deadzone, 0.0..=0.5).text("Center Deadzone"));
    ui.add(Slider::new(&mut tuning.saturation, 0.0..=0.5).text("End Deadzone"));
    ui.add(
        Slider::new(&mut tuning.curve, 0.2..=5.0)
            .logarithmic(true)
            .text("Curve"),
    )
    .on_hover_text("Above 1 is gentler near the center");
    ui.add(
        Slider::new(&mut tuning.scale, 0.1..=5.0)
            .logarithmic(true)
            .text(if relative { "Speed" } else { "Sensitivity" }),
    );

    if !relative {
        let mut limited = tuning.range.is_some();
        ui.checkbox(&mut limited, "Covers Partial Range")
            .on_hover_text("E.g. only looking one way with a stick");
        match (limited, tuning.range) {
            (true, None) => tuning.range = Some([0.0, 1.0]),
            (false, Some(_)) => tuning.range = None,
            _ => {}
        }
        if let Some([low, high]) = &mut tuning.range {
            ui.horizontal(|ui| {
                ui.label("From");
                ui.add(
                    DragValue::new(low)
                        .range(-1.0..=1.0)
                        .speed(0.01)
                        .fixed_decimals(2),
                );
                ui.label("to");
                ui.add(
                    DragValue::new(high)
                        .range(-1.0..=1.0)
                        .speed(0.01)
                        .fixed_decimals(2),
                );
            });
        }
    }

    curve_plot(ui, tuning, reading, scale);
}

fn curve_plot(ui: &mut Ui, tuning: &AxisTuning, reading: Option<f64>, scale: f32) {
    let size = 90.0 * scale;
    let (rect, _) = ui.allocate_exact_size(vec2(size, size), Sense::hover());
    if !ui.is_rect_visible(rect) {
        return;
    }
    let visuals = ui.visuals();
    let painter = ui.painter_at(rect);
    painter.rect_filled(rect, 1.0, visuals.extreme_bg_color);
    let guide = Stroke::new(1.0_f32, visuals.widgets.noninteractive.bg_stroke.color);
    painter.line_segment([rect.center_top(), rect.center_bottom()], guide);
    painter.line_segment([rect.left_center(), rect.right_center()], guide);

    let output = |input: f64| {
        let shaped = tuning.shape(input);
        match tuning.mode {
            AxisMode::Absolute => tuning.absolute(shaped),
            AxisMode::Relative => shaped,
        }
    };
    let to_screen = |input: f64, output: f64| -> Pos2 {
        let x = rect.center().x + input as f32 * rect.width() / 2.0;
        let y = rect.center().y - output.clamp(-1.0, 1.0) as f32 * rect.height() / 2.0;
        pos2(x, y)
    };
    const POINTS: usize = 64;
    let points: Vec<Pos2> = (0..=POINTS)
        .map(|n| {
            let input = n as f64 / POINTS as f64 * 2.0 - 1.0;
            to_screen(input, output(input))
        })
        .collect();
    painter.add(Shape::line(
        points,
        Stroke::new(1.5_f32, visuals.selection.bg_fill),
    ));

    if let Some(reading) = reading {
        painter.circle_filled(
            to_screen(reading, output(reading)),
            2.5 * scale,
            visuals.warn_fg_color,
        );
    }
}

fn axis_reading(editor: &Editor, devices: &[Device], source: &Source) -> Option<f64> {
    let Source::Axis { device, axis } = source else {
        return None;
    };
    let device = slot_device(editor, devices, device)?;
    let axis = device.axes.iter().find(|a| a.kind == *axis)?;
    Some(f64::from(axis.value()))
}

fn device_controls(ui: &mut Ui, device: &Device, scale: f32, text: &impl Fn(String) -> RichText) {
    for (index, axis) in device.axes.iter().enumerate() {
        ui.horizontal(|ui| {
            ui.add_sized(
                vec2(60.0 * scale, 10.0 * scale),
                egui::Label::new(text(format!("{}: {}", index + 1, axis.kind))),
            )
            .on_hover_text(&axis.name);
            ui.add(
                ProgressBar::new((axis.value() + 1.0) / 2.0)
                    .desired_width(160.0 * scale)
                    .desired_height(8.0 * scale)
                    .text(text(axis.raw.to_string())),
            );
        });
    }

    for (index, hat) in device.hats.iter().enumerate() {
        let direction = hat.direction.map_or("centered", HatDirection::name);
        ui.label(text(format!("Hat {}: {direction}", index + 1)))
            .on_hover_text(&hat.name);
    }

    button_grid(ui, device, scale);
}

fn button_grid(ui: &mut Ui, device: &Device, scale: f32) {
    let size = 12.0 * scale;
    let font = FontId::new(5.0 * scale, FontFamily::Name(FONT_FAMILY.into()));
    let visuals = ui.visuals().clone();

    ui.horizontal_wrapped(|ui| {
        ui.spacing_mut().item_spacing = vec2(2.0 * scale, 2.0 * scale);
        for (index, button) in device.buttons.iter().enumerate() {
            let (rect, response) = ui.allocate_exact_size(vec2(size, size), Sense::hover());
            let fill = if button.pressed {
                visuals.selection.bg_fill
            } else {
                visuals.widgets.inactive.bg_fill
            };
            let painter = ui.painter();
            painter.rect_filled(rect, 1.0, fill);
            painter.text(
                rect.center(),
                Align2::CENTER_CENTER,
                (index + 1).to_string(),
                font.clone(),
                visuals.text_color(),
            );
            response.on_hover_text(&button.name);
        }
    });
}
