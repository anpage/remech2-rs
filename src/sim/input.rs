use std::{collections::VecDeque, ffi::CStr, ptr, sync::Mutex};

use remech2_sys::sim::{self, InputSink};

use crate::input::{
    action::{ActionId, ActionKind, AxisAction, Context, Target},
    eval::{Evaluator, InputFrame},
    kbm,
    matching::{Assignment, match_slots},
    pad,
    profile::DeviceSlot,
    store,
};

use super::ticks::TICKS_PER_SECOND;

/// Analog sink positions run from -65536 to 65536
const POSITION_END: f64 = 65536.0;

/// The key code for Alt+Enter
const KEY_CODE_FULLSCREEN: i16 = 0x40d;

const KEY_CODE_ANY: i16 = 0x13;

const KEY_CODE_ESCAPE: i16 = 0x1b;

const GAME_KEY_START_UP_MECH: u8 = 61;
const GAME_KEY_SHUT_DOWN_MECH: u8 = 62;

const POWER_SHUT_DOWN: i32 = 3;

const MAX_GAME_KEY_REPEATS: u32 = 4;

const SETTLE_FRAMES: u32 = 2;

fn sink_name(sink: &InputSink) -> &CStr {
    unsafe { CStr::from_ptr(sink.m_name) }
}

fn sink_range(sink: &InputSink) -> f64 {
    f64::from(sink.m_max) - f64::from(sink.m_min)
}

fn sink_unit(sink: &InputSink) -> f64 {
    f64::from(1u32 << (16 - sink.m_outputShift.clamp(0, 16)))
}

fn sink_rest(sink: &InputSink) -> f64 {
    (f64::from(sink.m_rest) - f64::from(sink.m_min)) * 2.0 * POSITION_END / sink_range(sink)
        - POSITION_END
}

fn output_at(sink: &InputSink, position: f64) -> i32 {
    let t = (position.clamp(-POSITION_END, POSITION_END) + POSITION_END) / (2.0 * POSITION_END);
    ((f64::from(sink.m_min) + sink_range(sink) * t) * sink_unit(sink)).round() as i32
}

fn position_of(sink: &InputSink, output: i32) -> f64 {
    let value = f64::from(output) / sink_unit(sink);
    (value - f64::from(sink.m_min)) * 2.0 * POSITION_END / sink_range(sink) - POSITION_END
}

fn sinks() -> &'static [InputSink] {
    unsafe { &*ptr::addr_of!(sim::g_inputSinks) }
}

fn find_sink(name: &str) -> Option<usize> {
    let index = sinks().iter().position(|sink| {
        sink_name(sink)
            .to_bytes()
            .eq_ignore_ascii_case(name.as_bytes())
    });
    if index.is_none() {
        tracing::error!("The sim has no input sink {name:?}");
    }
    index
}

fn raised(index: usize) -> bool {
    unsafe { *sinks()[index].m_output.cast::<i8>() != 0 }
}

fn set_raised(index: usize, on: bool) {
    unsafe { *sinks()[index].m_output.cast::<i8>() = on.into() };
}

fn output(index: usize) -> i32 {
    unsafe { *sinks()[index].m_output.cast::<i32>() }
}

fn set_output(index: usize, value: i32) {
    unsafe { *sinks()[index].m_output.cast::<i32>() = value };
}

fn read_key_code() -> i16 {
    let mut code = 0;
    unsafe { sim::KeyboardReadKeyCode(&mut code) };
    if code == KEY_CODE_FULLSCREEN { 0 } else { code }
}

fn local_mech_shut_down() -> Option<bool> {
    let mech = unsafe { sim::g_localPlayer.as_ref()?.m_mech.as_ref()? };
    Some(mech.m_powerState == POWER_SHUT_DOWN)
}

fn game_key_applies(game_key: u8) -> bool {
    match game_key {
        GAME_KEY_START_UP_MECH => local_mech_shut_down() == Some(true),
        GAME_KEY_SHUT_DOWN_MECH => local_mech_shut_down() == Some(false),
        _ => true,
    }
}

struct AxisSink {
    action: ActionId,
    axis: &'static AxisAction,
    sink: usize,
    reset: Option<usize>,
    set: Option<usize>,
}

#[derive(Clone, Copy)]
struct Mode {
    context: Context,
    typing: bool,
}

impl Mode {
    fn current() -> Self {
        Self {
            context: Context {
                gameplay: unsafe { sim::g_gameplayInputEnabled } != 0,
                menu: !unsafe { sim::GetOpenMenu() }.is_null(),
            },
            typing: unsafe { sim::g_chatRecipient } != 0,
        }
    }

    fn interrupted(self) -> bool {
        !self.context.gameplay || self.typing
    }
}

struct SimInput {
    evaluator: Evaluator,
    slots: Vec<DeviceSlot>,
    assignment: Assignment,
    buttons: Vec<(ActionId, usize)>,
    axes: Vec<AxisSink>,
    game_keys: Vec<(ActionId, u8)>,
    menu_keys: Vec<(ActionId, i16)>,
    pause: Option<ActionId>,
    menu_back: Option<ActionId>,
    menu_key: Option<i16>,
    queued: VecDeque<u8>,
    asked: bool,
    was_interrupted: bool,
    settling: Option<(Mode, u32)>,
}

impl SimInput {
    fn new() -> Self {
        let (name, profile) = store::load_active_profile();
        tracing::info!("Using input profile {name:?}");
        let (mut evaluator, problems) = Evaluator::new(&profile);

        for problem in problems {
            tracing::warn!("Input profile {name:?}: {problem}");
        }

        evaluator.hold_sticks_until_moved();

        let sink_of = |id: ActionId| match id.action().target {
            Target::Sink(sink) => find_sink(sink),
            Target::GameKey(_) | Target::MenuKey(_) => None,
        };

        let mut buttons = Vec::new();
        let mut axes = Vec::new();
        let mut game_keys = Vec::new();
        let mut menu_keys = Vec::new();
        for id in ActionId::all() {
            let action = id.action();
            match (&action.kind, action.target) {
                (_, Target::GameKey(code)) => game_keys.push((id, code)),
                (_, Target::MenuKey(code)) => menu_keys.push((id, code)),
                (ActionKind::Axis(axis), _) => {
                    let Some(sink) = sink_of(id) else { continue };
                    let rest = sink_rest(&sinks()[sink]) / POSITION_END;

                    if (rest - axis.rest).abs() > 1e-6 {
                        tracing::error!(
                            "{} rests at {rest} in the sim, not {}",
                            action.name,
                            axis.rest
                        );
                    }

                    axes.push(AxisSink {
                        action: id,
                        axis,
                        sink,
                        reset: axis.reset.and_then(ActionId::find).and_then(sink_of),
                        set: axis.set_sink.and_then(find_sink),
                    });
                }
                (ActionKind::Hold | ActionKind::OneShot, _) => {
                    buttons.extend(sink_of(id).map(|sink| (id, sink)));
                    let also = action.also.and_then(find_sink);
                    buttons.extend(also.map(|sink| (id, sink)));
                }
            }
        }

        Self {
            evaluator,
            slots: profile.devices,
            assignment: Assignment::default(),
            buttons,
            axes,
            game_keys,
            menu_keys,
            pause: ActionId::find("pause"),
            menu_back: ActionId::find("menu_back"),
            menu_key: None,
            queued: VecDeque::new(),
            asked: false,
            was_interrupted: false,
            settling: None,
        }
    }

    fn settle(&mut self) -> (Mode, i16) {
        let mode = Mode::current();
        if mode.interrupted() {
            if !self.was_interrupted {
                unsafe { sim::KeyboardFlushKeyCodes() };
            }
            self.settling = Some((mode, SETTLE_FRAMES));
        }
        self.was_interrupted = mode.interrupted();
        let code = read_key_code();

        match self.settling {
            Some((left, frames)) if !mode.interrupted() => {
                self.settling = (frames > 1).then_some((left, frames - 1));
                (left, code)
            }
            _ => (mode, code),
        }
    }

    fn update(&mut self) -> i16 {
        let (mode, mut code) = self.settle();

        for axis in &self.axes {
            if axis.reset.is_some_and(raised) {
                self.evaluator.move_axis(axis.action, 0.0);
            } else if axis.set.is_some_and(raised) {
                let position = position_of(&sinks()[axis.sink], output(axis.sink));
                let value = axis.axis.value_at(position / POSITION_END);
                self.evaluator.move_axis(axis.action, value);
            }
        }
        for (index, sink) in sinks().iter().enumerate() {
            if sink.m_kind != sim::c_inputSinkAxis as i32 {
                set_raised(index, false);
            }
        }

        let kbm = kbm::snapshot();
        let pads = pad::snapshot();
        self.assignment = match_slots(&self.slots, &pads.devices, &self.assignment);
        let frame = InputFrame {
            kbm: &kbm,
            devices: &pads.devices,
            assignment: &self.assignment,
            typing: mode.typing,
        };
        let seconds = f64::from(unsafe { sim::g_deltaTime }) / TICKS_PER_SECOND;
        let state = self.evaluator.update(&frame, seconds, mode.context);

        for &(id, index) in &self.buttons {
            let output = state[id];
            let on = match id.action().kind {
                ActionKind::OneShot => output.presses > 0,
                _ => output.held,
            };
            if on {
                set_raised(index, true);
            }
        }

        for axis in &self.axes {
            let position = axis.axis.sink_position(state[axis.action].value) * POSITION_END;
            set_output(axis.sink, output_at(&sinks()[axis.sink], position));
        }

        if !std::mem::take(&mut self.asked) {
            self.queued.clear();
        }

        for &(id, game_key) in &self.game_keys {
            let most = match game_key {
                GAME_KEY_START_UP_MECH | GAME_KEY_SHUT_DOWN_MECH => 1,
                _ => MAX_GAME_KEY_REPEATS,
            };
            let presses = state[id].presses.min(most);
            if presses > 0 && game_key_applies(game_key) {
                self.queued.extend((0..presses).map(|_| game_key));
            }
        }

        let pressed = |id: ActionId| state[id].presses > 0;
        self.menu_key = self
            .menu_keys
            .iter()
            .find(|&&(id, _)| pressed(id))
            .map(|&(_, code)| code)
            .or_else(|| {
                self.menu_back
                    .is_some_and(pressed)
                    .then_some(KEY_CODE_ESCAPE)
            });

        let paused = unsafe { sim::g_simPaused != 0 && sim::g_pauseRequested != 0 };
        if paused && code == 0 && self.pause.is_some_and(pressed) {
            code = KEY_CODE_ANY;
        }
        code
    }
}

static SIM_INPUT: Mutex<Option<SimInput>> = Mutex::new(None);

#[unsafe(export_name = "FirstInputs")]
pub extern "C" fn first_inputs() {
    unsafe { sim::KeyboardFlushKeyCodes() };
    *SIM_INPUT.lock().unwrap() = Some(SimInput::new());
}

#[unsafe(export_name = "UpdateInputs")]
pub extern "C" fn update_inputs() {
    let mut buttons = 0;
    unsafe { sim::MousePoll(ptr::null_mut(), ptr::null_mut(), &mut buttons) };

    let code = match SIM_INPUT.lock().unwrap().as_mut() {
        Some(input) => input.update(),
        None => read_key_code(),
    };
    unsafe { sim::g_localSteering.m_keyCode = code };
}

#[unsafe(export_name = "LookupGameKey")]
pub extern "C" fn lookup_game_key(_key_code: i16) -> i16 {
    let mut input = SIM_INPUT.lock().unwrap();
    let Some(input) = input.as_mut() else {
        return 0;
    };
    input.asked = true;
    input.queued.pop_front().map_or(0, i16::from)
}

#[unsafe(export_name = "TakeBoundMenuKey")]
pub extern "C" fn take_bound_menu_key() -> i16 {
    SIM_INPUT
        .lock()
        .unwrap()
        .as_mut()
        .and_then(|input| input.menu_key.take())
        .unwrap_or(0)
}

#[unsafe(export_name = "CloseInputDevices")]
pub extern "C" fn close_inputs() {
    SIM_INPUT.lock().unwrap().take();
}
