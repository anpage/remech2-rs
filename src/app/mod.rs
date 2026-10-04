mod renderer;

use std::{cell::RefCell, sync::Arc, thread, time::Duration};

use anyhow::{Result, bail};
use winit::{
    application::ApplicationHandler,
    dpi::PhysicalSize,
    event::{ElementState, KeyEvent, WindowEvent},
    event_loop::{ActiveEventLoop, EventLoop},
    keyboard::{Key, ModifiersState, NamedKey},
    platform::pump_events::{EventLoopExtPumpEvents, PumpStatus},
    window::{Fullscreen, Window, WindowId},
};

use crate::settings::SETTINGS;

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
        let status = self
            .event_loop
            .pump_app_events(Some(Duration::ZERO), &mut self.state);
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
        if let (Some(window), Some(egui_input)) = (&self.window, &mut self.egui_input) {
            let _ = egui_input.on_window_event(window, &event);
        }

        match event {
            WindowEvent::CloseRequested => self.quit = true,
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
            _ => {}
        }
    }
}
