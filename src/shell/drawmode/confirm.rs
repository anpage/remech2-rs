use std::sync::Mutex;

use egui::{Align, Button, Color32, Context, FontId, Layout, RichText, Vec2};

const BUTTON_WIDTH: f32 = 64.0;
const BUTTON_GAP: f32 = 42.0;
const TEXT_SIZE: f32 = 16.0;
const MARGIN: f32 = 20.0;

struct Prompt {
    lines: Vec<String>,
    buttons: Buttons,
    answer: Option<bool>,
}

#[derive(Clone, Copy)]
enum Buttons {
    YesNo,
    Ok,
}

impl Buttons {
    fn labels(self) -> &'static [&'static str] {
        match self {
            Self::YesNo => &["Yes", "No"],
            Self::Ok => &["Ok"],
        }
    }
}

static PROMPT: Mutex<Option<Prompt>> = Mutex::new(None);

/// Opens a Yes/No prompt, replacing any open one.
pub fn open(lines: &[&str]) {
    set(lines, Buttons::YesNo);
}

/// Opens a message with a single `Ok`, replacing any open prompt.
/// [`answer`] reports `Some(true)` once it's acknowledged.
pub fn alert(lines: &[&str]) {
    set(lines, Buttons::Ok);
}

fn set(lines: &[&str], buttons: Buttons) {
    *PROMPT.lock().unwrap() = Some(Prompt {
        lines: lines.iter().map(|&line| line.to_owned()).collect(),
        buttons,
        answer: None,
    });
}

/// `Some(true)` for Yes, `Some(false)` for No. `None` while unanswered, or if nothing is open.
pub fn answer() -> Option<bool> {
    PROMPT
        .lock()
        .unwrap()
        .as_ref()
        .and_then(|prompt| prompt.answer)
}

pub fn close() {
    *PROMPT.lock().unwrap() = None;
}

/// Draws the open prompt until it's answered.
pub(super) fn window(ctx: &Context, scale: f32) {
    let mut prompt = PROMPT.lock().unwrap();
    let Some(prompt) = prompt.as_mut().filter(|prompt| prompt.answer.is_none()) else {
        return;
    };
    prompt.answer = render(ctx, &prompt.lines, prompt.buttons, scale);
}

/// Width of one line of text, for sizing the window to its contents.
fn text_width(ctx: &Context, text: &str, font: FontId) -> f32 {
    ctx.fonts_mut(|fonts| {
        fonts
            .layout_no_wrap(text.to_owned(), font, Color32::PLACEHOLDER)
            .size()
            .x
    })
}

/// A Yes/No dialog. Returns the button clicked this frame.
pub(super) fn dialog(ctx: &Context, lines: &[&str], scale: f32) -> Option<bool> {
    render(ctx, lines, Buttons::YesNo, scale)
}

/// Draws a dialog sized to its contents. Returns `true` for the leftmost button.
fn render<S: AsRef<str>>(ctx: &Context, lines: &[S], buttons: Buttons, scale: f32) -> Option<bool> {
    let button_width = BUTTON_WIDTH * scale;
    let button_gap = BUTTON_GAP * scale;
    let text_size = TEXT_SIZE * scale;
    let margin = MARGIN * scale;

    let labels = buttons.labels();
    let gaps = labels.len().saturating_sub(1) as f32;
    let row_width = labels.len() as f32 * button_width + gaps * button_gap;

    let font = FontId::proportional(text_size);
    let content_width = lines
        .iter()
        .map(|line| text_width(ctx, line.as_ref(), font.clone()))
        .chain([row_width])
        .fold(0.0, f32::max);

    let mut answer = None;
    egui::Window::new("shell_dialog")
        .title_bar(false)
        .resizable(false)
        .collapsible(false)
        .movable(false)
        .pivot(egui::Align2::CENTER_CENTER)
        .fixed_pos(ctx.content_rect().center())
        .show(ctx, |ui| {
            egui::Frame::new().inner_margin(margin).show(ui, |ui| {
                ui.set_width(content_width);
                ui.vertical_centered(|ui| {
                    for line in lines {
                        ui.label(RichText::new(line.as_ref()).size(text_size));
                    }
                    ui.add_space(text_size);

                    let row = Vec2::new(row_width, ui.spacing().interact_size.y);
                    ui.allocate_ui_with_layout(row, Layout::left_to_right(Align::Center), |ui| {
                        ui.spacing_mut().item_spacing.x = button_gap;
                        for (i, label) in labels.iter().enumerate() {
                            let button = Button::new(RichText::new(*label).size(text_size))
                                .min_size(Vec2::new(button_width, 0.0));
                            if ui.add(button).clicked() {
                                answer = Some(i == 0);
                            }
                        }
                    });
                });
            });
        });
    answer
}
