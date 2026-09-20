use std::sync::Mutex;

use egui::{Align, Button, Color32, Context, FontId, Layout, RichText, TextStyle, Vec2};

const BUTTON_WIDTH: f32 = 64.0;
const BUTTON_GAP: f32 = 42.0;
const TEXT_SIZE: f32 = 16.0;
const TITLE_BAR_PADDING: f32 = 24.0;

struct Prompt {
    title: String,
    body: Vec<String>,
    answer: Option<bool>,
}

static PROMPT: Mutex<Option<Prompt>> = Mutex::new(None);

/// Opens a prompt, replacing any open one.
pub fn open(title: &str, body: &[&str]) {
    *PROMPT.lock().unwrap() = Some(Prompt {
        title: (*title).to_owned(),
        body: body.iter().map(|&line| line.to_owned()).collect(),
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
pub(super) fn window(ctx: &Context) {
    let mut prompt = PROMPT.lock().unwrap();
    let Some(prompt) = prompt.as_mut().filter(|prompt| prompt.answer.is_none()) else {
        return;
    };
    prompt.answer = dialog(ctx, &prompt.title, &prompt.body);
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
pub(super) fn dialog(ctx: &Context, title: &str, body: &[String]) -> Option<bool> {
    let row_width = 2.0 * BUTTON_WIDTH + BUTTON_GAP;

    let body_font = FontId::proportional(TEXT_SIZE);
    let title_font = TextStyle::Heading.resolve(&ctx.style());
    let content_width = body
        .iter()
        .map(|line| text_width(ctx, line, body_font.clone()))
        .chain([
            row_width,
            text_width(ctx, title, title_font) + TITLE_BAR_PADDING,
        ])
        .fold(0.0, f32::max);

    let mut answer = None;
    egui::Window::new(title)
        .resizable(false)
        .collapsible(false)
        .pivot(egui::Align2::CENTER_CENTER)
        .fixed_pos(ctx.content_rect().center())
        .show(ctx, |ui| {
            egui::Frame::new().inner_margin(20.0).show(ui, |ui| {
                ui.set_width(content_width);
                ui.vertical_centered(|ui| {
                    for line in body {
                        ui.label(RichText::new(line).size(TEXT_SIZE));
                    }

                    let row = Vec2::new(row_width, ui.spacing().interact_size.y);
                    ui.allocate_ui_with_layout(row, Layout::left_to_right(Align::Center), |ui| {
                        ui.spacing_mut().item_spacing.x = BUTTON_GAP;
                        let button = |text| {
                            Button::new(RichText::new(text).size(TEXT_SIZE))
                                .min_size(Vec2::new(BUTTON_WIDTH, 0.0))
                        };
                        if ui.add(button("Yes")).clicked() {
                            answer = Some(true);
                        }
                        if ui.add(button("No")).clicked() {
                            answer = Some(false);
                        }
                    });
                });
            });
        });
    answer
}
