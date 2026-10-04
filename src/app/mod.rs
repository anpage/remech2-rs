mod keyboard;
mod renderer;

use std::{cell::RefCell, ffi::c_int, process::exit, sync::Arc, thread, time::Duration};

use anyhow::{Result, bail};
use winit::{
    application::ApplicationHandler,
    dpi::PhysicalSize,
    event::{ElementState, KeyEvent, WindowEvent},
    event_loop::{ActiveEventLoop, EventLoop},
    keyboard::{Key, ModifiersState, NamedKey, PhysicalKey},
    platform::{
        modifier_supplement::KeyEventExtModifierSupplement,
        pump_events::{EventLoopExtPumpEvents, PumpStatus},
    },
    window::{Fullscreen, Window, WindowId},
};

use mw2_sys::shared::{c_mechMsgActivateApp, c_mechMsgKeyDown, c_mechMsgKeyUp};

use crate::{messages, settings::SETTINGS};

pub use renderer::Frame;
use renderer::Renderer;

thread_local! {
    static APP: RefCell<Option<App>> = const { RefCell::new(None) };
}

pub fn install(app: App) {
    APP.set(Some(app));
}

pub fn with<R>(f: impl FnOnce(&mut App) -> R) -> Option<R> {
    APP.with(|app| match app.try_borrow_mut() {
        Ok(mut app) => app.as_mut().map(f),
        Err(_) => {
            tracing::warn!("the app is already in use");
            None
        }
    })
}

/// The app's one window
pub struct App {
    event_loop: EventLoop<()>,
    state: State,
}

impl App {
    /// Opens the window
    pub fn new() -> Result<Self> {
        let mut app = Self {
            event_loop: EventLoop::new()?,
            state: State::new(),
        };
        // The window is created when the event loop first runs
        app.pump();
        if let Some(error) = app.state.error.take() {
            return Err(error);
        }
        if app.state.window.is_none() {
            bail!("the event loop did not start");
        }
        Ok(app)
    }

    /// Handles the window events that are waiting, without blocking.
    /// Returns whether the window was closed and the caller should quit.
    pub fn pump(&mut self) -> bool {
        self.pump_events(Some(Duration::ZERO))
    }

    /// Like `pump`, but waits for an event first
    pub fn wait(&mut self) -> bool {
        self.pump_events(None)
    }

    fn pump_events(&mut self, timeout: Option<Duration>) -> bool {
        let status = self.event_loop.pump_app_events(timeout, &mut self.state);
        if matches!(status, PumpStatus::Exit(_)) {
            self.state.quit = true;
        }
        self.state.quit
    }

    pub fn egui_ctx(&self) -> &egui::Context {
        &self.state.egui_ctx
    }

    /// Draws the game's frame with `ui` over it and waits for it to be shown
    pub fn present(&mut self, frame: Option<Frame>, ui: impl FnMut(&egui::Context)) {
        self.state.present(frame, ui);
    }
}

struct State {
    window: Option<Arc<Window>>,
    renderer: Option<Renderer>,
    egui_ctx: egui::Context,
    egui_input: Option<egui_winit::State>,
    modifiers: ModifiersState,
    focused: bool,
    quit: bool,
    error: Option<anyhow::Error>,
}

impl State {
    fn new() -> Self {
        Self {
            window: None,
            renderer: None,
            egui_ctx: egui::Context::default(),
            egui_input: None,
            modifiers: ModifiersState::empty(),
            focused: false,
            quit: false,
            error: None,
        }
    }

    fn create_window(&mut self, event_loop: &ActiveEventLoop) -> Result<()> {
        let fullscreen = SETTINGS.get_bool("video", "fullscreen", true);
        let width = SETTINGS.get_int("video", "width", 1024).max(1) as u32;
        let height = SETTINGS.get_int("video", "height", 768).max(1) as u32;

        let attributes = Window::default_attributes()
            .with_title("REMECH 2")
            .with_inner_size(PhysicalSize::new(width, height))
            .with_fullscreen(fullscreen.then_some(Fullscreen::Borderless(None)));
        let window = Arc::new(event_loop.create_window(attributes)?);

        let renderer = Renderer::new(window.clone())?;

        self.egui_input = Some(egui_winit::State::new(
            self.egui_ctx.clone(),
            egui::ViewportId::ROOT,
            &window,
            Some(window.scale_factor() as f32),
            window.theme(),
            Some(renderer.max_texture_side()),
        ));
        self.window = Some(window);
        self.renderer = Some(renderer);
        Ok(())
    }

    fn present(&mut self, frame: Option<Frame>, ui: impl FnMut(&egui::Context)) {
        let (Some(window), Some(renderer), Some(egui_input)) =
            (&self.window, &mut self.renderer, &mut self.egui_input)
        else {
            return;
        };

        // Minimized
        let size = window.inner_size();
        if size.width == 0 || size.height == 0 {
            thread::sleep(Duration::from_millis(16));
            return;
        }

        let raw_input = egui_input.take_egui_input(window);
        let full_output = self.egui_ctx.run(raw_input, ui);
        egui_input.handle_platform_output(window, full_output.platform_output);

        let primitives = self
            .egui_ctx
            .tessellate(full_output.shapes, full_output.pixels_per_point);
        renderer.render(
            frame,
            &primitives,
            &full_output.textures_delta,
            full_output.pixels_per_point,
        );
    }

    fn toggle_fullscreen(&self) {
        let Some(window) = &self.window else {
            return;
        };
        let fullscreen = window.fullscreen().is_none();
        SETTINGS.set_bool("video", "fullscreen", fullscreen);
        window.set_fullscreen(fullscreen.then_some(Fullscreen::Borderless(None)));
    }
}

impl ApplicationHandler for State {
    fn resumed(&mut self, event_loop: &ActiveEventLoop) {
        if self.window.is_some() {
            return;
        }
        if let Err(error) = self.create_window(event_loop) {
            self.error = Some(error);
            self.quit = true;
        }
    }

    fn window_event(
        &mut self,
        _event_loop: &ActiveEventLoop,
        _window_id: WindowId,
        event: WindowEvent,
    ) {
        let mut consumed = false;
        if let (Some(window), Some(egui_input)) = (&self.window, &mut self.egui_input) {
            consumed = egui_input.on_window_event(window, &event).consumed;
        }

        match event {
            WindowEvent::CloseRequested => self.quit = true,
            WindowEvent::Focused(focused) => {
                self.focused = focused;
                messages::post(c_mechMsgActivateApp as u32, focused.into(), 0);
            }
            WindowEvent::Resized(size) => {
                if let Some(renderer) = &mut self.renderer {
                    renderer.resize(size.width, size.height);
                }
            }
            WindowEvent::ModifiersChanged(modifiers) => self.modifiers = modifiers.state(),
            WindowEvent::KeyboardInput {
                event:
                    KeyEvent {
                        logical_key: Key::Named(NamedKey::Enter),
                        state: ElementState::Pressed,
                        repeat: false,
                        ..
                    },
                ..
            } if self.modifiers.alt_key() => self.toggle_fullscreen(),
            // Releases always reach the game so that no key is left held down
            WindowEvent::KeyboardInput { event, .. }
                if !consumed || event.state == ElementState::Released =>
            {
                post_key(&event);
            }
            _ => {}
        }
    }
}

/// Posts a key event to the game's message queue
fn post_key(event: &KeyEvent) {
    let PhysicalKey::Code(code) = event.physical_key else {
        return;
    };
    let character = match event.key_without_modifiers() {
        Key::Character(text) => text.chars().next(),
        _ => None,
    };
    let Some((wparam, lparam)) = keyboard::params(code, character) else {
        return;
    };

    let message = match event.state {
        ElementState::Pressed => c_mechMsgKeyDown,
        ElementState::Released => c_mechMsgKeyUp,
    };
    messages::post(message, wparam, lparam);
}

#[unsafe(export_name = "MechAppPump")]
pub extern "C" fn pump() {
    if with(App::pump) == Some(true) {
        exit(0);
    }
}

#[unsafe(export_name = "MechAppWait")]
pub extern "C" fn wait() {
    if with(App::wait) == Some(true) {
        exit(0);
    }
}

#[unsafe(export_name = "MechAppActive")]
pub extern "C" fn active() -> c_int {
    with(|app| app.state.focused).unwrap_or(false).into()
}
