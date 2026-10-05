use std::{fmt, ops::Index};

use super::{
    KeyCode,
    action::{ActionId, ActionKind, AxisBehaviour, Context, RAMP_SPEED_MAX, list::ACTIONS},
    binding::{AxisMode, AxisTuning, Binding},
    kbm::{KbmState, MouseButton, WheelDirection},
    matching::Assignment,
    pad::{AxisKind, Device, HatDirection},
    profile::Profile,
    source::{Modifier, MouseAxis, Source, SourceKind, Threshold},
};

const MOUSE_COUNTS_PER_UNIT: f64 = 1000.0;

/// How far notches in the mouse wheel move an axis
const WHEEL_STEP: f64 = 0.1;

/// How far an absolute axis has to move to take its action backfrom a relative input
const TAKEOVER_DISTANCE: f64 = 0.05;

/// The longest frame we integrate over
const MAX_FRAME_SECONDS: f64 = 0.1;

pub struct InputFrame<'a> {
    pub kbm: &'a KbmState,
    pub devices: &'a [Device],
    pub assignment: &'a Assignment,
    /// The keyboard is typing text
    pub typing: bool,
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ActionOutput {
    pub held: bool,
    /// Number of presses this frame
    pub presses: u32,
    pub value: f64,
}

#[derive(Clone, Debug)]
pub struct ActionState {
    outputs: Vec<ActionOutput>,
}

impl Default for ActionState {
    fn default() -> Self {
        Self {
            outputs: vec![ActionOutput::default(); ACTIONS.len()],
        }
    }
}

impl Index<ActionId> for ActionState {
    type Output = ActionOutput;

    fn index(&self, id: ActionId) -> &ActionOutput {
        &self.outputs[id.index()]
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct Problem {
    pub binding: usize,
    pub message: String,
}

impl fmt::Display for Problem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Binding {}: {}", self.binding + 1, self.message)
    }
}

#[derive(Clone, Debug, PartialEq)]
enum Input {
    Key(KeyCode),
    Modifier(Modifier),
    MouseButton(MouseButton),
    Wheel(WheelDirection),
    MouseAxis(MouseAxis),
    Button {
        slot: usize,
        index: usize,
    },
    Hat {
        slot: usize,
        index: usize,
        direction: HatDirection,
    },
    Axis {
        slot: usize,
        axis: AxisKind,
    },
    Threshold {
        slot: usize,
        axis: AxisKind,
        threshold: Threshold,
    },
}

#[derive(Clone, Copy, Debug)]
enum Role {
    Button {
        toggle: bool,
    },
    AbsoluteAxis,
    RelativeAxis,
    MouseAxis,
    /// Ramps while held or moves `step` per press
    AxisButton {
        sign: f64,
        step: Option<f64>,
    },
    /// Puts a position axis somewhere specific on each press
    AxisSetter {
        position: f64,
    },
}

struct Compiled {
    action: ActionId,
    trigger: Input,
    modifiers: Vec<Input>,
    /// Bindings on the same input that win over this one
    outranked_by: Vec<usize>,
    role: Role,
    tuning: AxisTuning,
    /// The trigger's counter last frame and the device it came from
    counter: Option<(Option<usize>, i64)>,
    /// Whether the binding was engaged and its input held last frame
    was_active: bool,
    toggled: bool,
    waiting: Option<Option<f64>>,
}

/// One frame's reading of an input
#[derive(Default)]
struct Reading {
    held: bool,
    counter: Option<(Option<usize>, i64)>,
    value: Option<f64>,
}

/// A binding's status this frame
struct Status {
    held: bool,
    delta: i64,
    value: Option<f64>,
    available: bool,
    chord: bool,
}

#[derive(Clone, Copy, Default)]
struct AxisInputs {
    /// The farthest-deflected absolute axis
    absolute: Option<(f64, f64)>,
    /// Speed from relative axes, in travel per second
    rate: f64,
    /// Sum of held ramp buttons' directions, weighted by their scale
    ramp: f64,
    /// Travel from steps and the mouse
    shift: f64,
    /// Where a setting button put it
    set: Option<f64>,
}

#[derive(Clone, Copy, Default)]
struct AxisRuntime {
    /// Where a position axis is or how far a rate axis has moved
    position: f64,
    /// How fast the button ramp is going
    ramp_speed: f64,
    /// Whether a relative input moved the axis last and where the absolute axes were then
    relative: Option<Option<f64>>,
    /// Where the absolute axes put it last frame
    absolute: Option<f64>,
}

pub struct Evaluator {
    slots: Vec<String>,
    bindings: Vec<Compiled>,
    /// By action index
    axes: Vec<AxisRuntime>,
    /// (axis, the action that resets it)
    resets: Vec<(ActionId, ActionId)>,
    state: ActionState,
}

impl Evaluator {
    pub fn new(profile: &Profile) -> (Self, Vec<Problem>) {
        let slots: Vec<String> = profile.devices.iter().map(|slot| slot.id.clone()).collect();
        let mut problems = Vec::new();
        let mut bindings = Vec::new();
        for (index, binding) in profile.bindings.iter().enumerate() {
            match compile(binding, &slots) {
                Ok(compiled) => bindings.push(compiled),
                Err(message) => problems.push(Problem {
                    binding: index,
                    message,
                }),
            }
        }

        for i in 0..bindings.len() {
            bindings[i].outranked_by = (0..bindings.len())
                .filter(|&j| {
                    bindings[j].trigger == bindings[i].trigger
                        && outranks(&bindings[j].modifiers, &bindings[i].modifiers)
                })
                .collect();
        }

        let resets = ActionId::all()
            .filter_map(|id| {
                let reset = id.action().axis()?.reset?;
                Some((id, ActionId::find(reset)?))
            })
            .collect();

        let evaluator = Self {
            slots,
            bindings,
            axes: vec![AxisRuntime::default(); ACTIONS.len()],
            resets,
            state: ActionState::default(),
        };
        (evaluator, problems)
    }

    pub fn state(&self) -> &ActionState {
        &self.state
    }

    /// Works out this frame's action state
    pub fn update(
        &mut self,
        frame: &InputFrame,
        delta_seconds: f64,
        context: Context,
    ) -> &ActionState {
        let dt = delta_seconds.clamp(0.0, MAX_FRAME_SECONDS);
        let devices: Vec<Option<&Device>> = self
            .slots
            .iter()
            .map(|slot| {
                let id = frame.assignment.device(slot)?;
                frame.devices.get(id).filter(|device| device.connected)
            })
            .collect();

        let statuses: Vec<Status> = self
            .bindings
            .iter_mut()
            .map(|binding| {
                let reading = read(&binding.trigger, frame.kbm, &devices);
                let motion = matches!(binding.trigger, Input::MouseAxis(_));
                let delta = take_delta(&mut binding.counter, reading.counter, motion);
                let typed = matches!(binding.trigger, Input::Key(_) | Input::Modifier(_));
                let available = binding.action.action().availability.includes(context)
                    && !(frame.typing && typed);
                let chord = available
                    && binding
                        .modifiers
                        .iter()
                        .all(|modifier| read(modifier, frame.kbm, &devices).held);
                Status {
                    held: reading.held,
                    delta,
                    value: reading.value,
                    available,
                    chord,
                }
            })
            .collect();

        let mut outputs = vec![ActionOutput::default(); ACTIONS.len()];
        let mut axis_inputs = vec![AxisInputs::default(); ACTIONS.len()];
        for (index, binding) in self.bindings.iter_mut().enumerate() {
            let status = &statuses[index];
            let engaged = status.chord && !binding.outranked_by.iter().any(|&j| statuses[j].chord);
            if !stick_counts(binding, status.value) {
                continue;
            }
            let output = &mut outputs[binding.action.index()];
            let inputs = &mut axis_inputs[binding.action.index()];
            let tuning = &binding.tuning;

            match binding.role {
                Role::Button { toggle } => {
                    let (active, presses) = digital(binding, status, engaged);
                    if toggle {
                        if presses % 2 == 1 {
                            binding.toggled = !binding.toggled;
                        }
                        output.held |= binding.toggled && status.available;
                    } else {
                        output.held |= active;
                    }
                    output.presses += presses;
                }
                Role::AbsoluteAxis => {
                    if let (true, Some(value)) = (engaged, status.value) {
                        let shaped = tuning.shape(value);
                        let deflection = shaped.abs();
                        let mut position = tuning.absolute(shaped);
                        let one_sided = binding
                            .action
                            .action()
                            .axis()
                            .is_some_and(|axis| axis.min == 0.0);
                        if one_sided && tuning.range.is_none() {
                            position = (position + 1.0) / 2.0;
                        }
                        if inputs
                            .absolute
                            .is_none_or(|(furthest, _)| deflection > furthest)
                        {
                            inputs.absolute = Some((deflection, position));
                        }
                    }
                }
                Role::RelativeAxis => {
                    if let (true, Some(value)) = (engaged, status.value) {
                        inputs.rate += tuning.shape(value) * tuning.scale;
                    }
                }
                Role::MouseAxis => {
                    if engaged {
                        let sign = if tuning.invert { -1.0 } else { 1.0 };
                        inputs.shift +=
                            sign * status.delta as f64 * tuning.scale / MOUSE_COUNTS_PER_UNIT;
                    }
                }
                Role::AxisButton { sign, step } => {
                    let (active, presses) = digital(binding, status, engaged);
                    match step {
                        Some(step) => inputs.shift += sign * step * f64::from(presses),
                        None if active => inputs.ramp += sign * binding.tuning.scale,
                        None => {}
                    }
                }
                Role::AxisSetter { position } => {
                    let (_, presses) = digital(binding, status, engaged);
                    if presses > 0 {
                        inputs.set = Some(position);
                    }
                }
            }
        }

        for id in ActionId::all() {
            let Some(axis) = id.action().axis() else {
                continue;
            };
            let inputs = axis_inputs[id.index()];
            let runtime = &mut self.axes[id.index()];

            if inputs.ramp == 0.0 || runtime.ramp_speed * inputs.ramp < 0.0 {
                runtime.ramp_speed = 0.0;
            }
            runtime.ramp_speed = (runtime.ramp_speed + inputs.ramp * axis.ramp_acceleration * dt)
                .clamp(-RAMP_SPEED_MAX, RAMP_SPEED_MAX);
            let ramp_travel = runtime.ramp_speed * dt;
            let absolute = inputs.absolute.map(|(_, value)| value);
            runtime.absolute = absolute;

            let value = match axis.behaviour {
                AxisBehaviour::Position { .. } => {
                    let relative_active =
                        inputs.rate != 0.0 || inputs.ramp != 0.0 || inputs.shift != 0.0;
                    if let Some(set) = inputs.set {
                        runtime.position = set;
                        runtime.ramp_speed = 0.0;
                        runtime.relative = Some(absolute);
                    } else if relative_active {
                        let ramped =
                            axis.value_at(axis.sink_position(runtime.position) + ramp_travel);
                        runtime.position = ramped + inputs.rate * dt + inputs.shift;
                        runtime.relative = Some(absolute);
                    } else if let Some(absolute) = absolute {
                        let takes_over = match runtime.relative {
                            None | Some(None) => true,
                            Some(Some(left_at)) => (absolute - left_at).abs() > TAKEOVER_DISTANCE,
                        };
                        if takes_over {
                            runtime.position = absolute;
                            runtime.relative = None;
                        }
                    }
                    runtime.position = runtime.position.clamp(axis.min, 1.0);
                    runtime.position
                }
                AxisBehaviour::Rate => {
                    runtime.position = if inputs.ramp == 0.0 {
                        0.0
                    } else {
                        (runtime.position + ramp_travel).clamp(-1.0, 1.0)
                    };
                    let shift_speed = if dt > 0.0 { inputs.shift / dt } else { 0.0 };
                    (absolute.unwrap_or(0.0) + runtime.position + shift_speed).clamp(-1.0, 1.0)
                }
            };
            outputs[id.index()].value = value;
        }

        self.state.outputs = outputs;
        for index in 0..self.resets.len() {
            let (axis, reset) = self.resets[index];
            let reset = self.state[reset];
            if reset.held || reset.presses > 0 {
                self.move_axis(axis, 0.0);
            }
        }
        &self.state
    }

    pub fn hold_sticks_until_moved(&mut self) {
        for binding in &mut self.bindings {
            if matches!(binding.role, Role::AbsoluteAxis | Role::RelativeAxis) {
                binding.waiting = Some(None);
            }
        }
    }

    pub fn move_axis(&mut self, id: ActionId, position: f64) {
        let Some(axis) = id.action().axis() else {
            return;
        };
        let runtime = &mut self.axes[id.index()];
        runtime.position = position.clamp(axis.min, 1.0);
        runtime.ramp_speed = 0.0;
        runtime.relative = Some(runtime.absolute);
        self.state.outputs[id.index()].value = runtime.position;
    }
}

fn outranks(a: &[Input], b: &[Input]) -> bool {
    a.len() > b.len() && b.iter().all(|input| a.contains(input))
}

fn stick_counts(binding: &mut Compiled, value: Option<f64>) -> bool {
    let Some(start) = binding.waiting else {
        return true;
    };
    let Some(value) = value else {
        return false;
    };
    match start {
        None => binding.waiting = Some(Some(value)),
        Some(start) if (value - start).abs() > TAKEOVER_DISTANCE => binding.waiting = None,
        Some(_) => {}
    }
    binding.waiting.is_none()
}

fn digital(binding: &mut Compiled, status: &Status, engaged: bool) -> (bool, u32) {
    if !engaged {
        binding.was_active = !status.available && status.held;
        return (false, 0);
    }
    let pulses = status.delta.max(0) as u32;
    let active = status.held || pulses > 0;
    let rising = active && !binding.was_active;
    binding.was_active = status.held;
    (active, pulses.max(u32::from(rising)))
}

fn take_delta(
    last: &mut Option<(Option<usize>, i64)>,
    now: Option<(Option<usize>, i64)>,
    motion: bool,
) -> i64 {
    let delta = match (*last, now) {
        (Some((last_device, before)), Some((device, after))) if last_device == device => {
            if motion {
                after - before
            } else {
                i64::from((after as u32).wrapping_sub(before as u32))
            }
        }
        _ => 0,
    };
    *last = now;
    delta
}

fn read(input: &Input, kbm: &KbmState, devices: &[Option<&Device>]) -> Reading {
    let presses = |count: u32| Some((None, i64::from(count)));
    match *input {
        Input::Key(key) => Reading {
            held: kbm.keys_held.contains(&key),
            counter: presses(kbm.key_presses(key)),
            value: None,
        },
        Input::Modifier(modifier) => {
            let [left, right] = modifier.keys();
            Reading {
                held: kbm.keys_held.contains(&left) || kbm.keys_held.contains(&right),
                counter: presses(kbm.key_presses(left).wrapping_add(kbm.key_presses(right))),
                value: None,
            }
        }
        Input::MouseButton(button) => Reading {
            held: kbm.button_held(button),
            counter: presses(kbm.button_presses(button)),
            value: None,
        },
        Input::Wheel(direction) => Reading {
            held: false,
            counter: presses(kbm.wheel_notches(direction)),
            value: None,
        },
        Input::MouseAxis(axis) => Reading {
            held: false,
            counter: Some((
                None,
                match axis {
                    MouseAxis::X => kbm.motion_total.0,
                    MouseAxis::Y => kbm.motion_total.1,
                },
            )),
            value: None,
        },
        Input::Button { slot, index } => {
            let Some((device, button)) =
                devices[slot].and_then(|device| Some((device, device.buttons.get(index)?)))
            else {
                return Reading::default();
            };
            Reading {
                held: button.pressed,
                counter: Some((Some(device.id), i64::from(button.press_count))),
                value: None,
            }
        }
        Input::Hat {
            slot,
            index,
            direction,
        } => Reading {
            held: devices[slot]
                .and_then(|device| device.hats.get(index)?.direction)
                .is_some_and(|pushed| pushed.includes(direction)),
            ..Default::default()
        },
        Input::Axis { slot, axis } => Reading {
            value: axis_value(devices[slot], axis),
            ..Default::default()
        },
        Input::Threshold {
            slot,
            axis,
            threshold,
        } => Reading {
            held: axis_value(devices[slot], axis).is_some_and(|value| threshold.reached(value)),
            ..Default::default()
        },
    }
}

fn axis_value(device: Option<&Device>, kind: AxisKind) -> Option<f64> {
    let axis = device?.axes.iter().find(|axis| axis.kind == kind)?;
    Some(f64::from(axis.value()))
}

fn resolve(source: &Source, slots: &[String]) -> Result<Input, String> {
    let slot = |device: &str| {
        slots
            .iter()
            .position(|slot| slot == device)
            .ok_or_else(|| format!("no device slot {device:?}"))
    };
    Ok(match source {
        Source::Key(key) => Input::Key(*key),
        Source::Modifier(modifier) => Input::Modifier(*modifier),
        Source::MouseButton(button) => Input::MouseButton(*button),
        Source::Wheel(direction) => Input::Wheel(*direction),
        Source::MouseAxis(axis) => Input::MouseAxis(*axis),
        Source::Button { device, index } => Input::Button {
            slot: slot(device)?,
            index: usize::from(*index),
        },
        Source::Hat {
            device,
            index,
            direction,
        } => Input::Hat {
            slot: slot(device)?,
            index: usize::from(*index),
            direction: *direction,
        },
        Source::Axis { device, axis } => Input::Axis {
            slot: slot(device)?,
            axis: *axis,
        },
        Source::AxisThreshold {
            device,
            axis,
            threshold,
        } => Input::Threshold {
            slot: slot(device)?,
            axis: *axis,
            threshold: *threshold,
        },
    })
}

fn compile(binding: &Binding, slots: &[String]) -> Result<Compiled, String> {
    let action = ActionId::find(&binding.action)
        .ok_or_else(|| format!("unknown action {:?}", binding.action))?;
    let trigger = resolve(&binding.source, slots)?;
    let modifiers = binding
        .modifiers
        .iter()
        .map(|modifier| match modifier {
            Source::Wheel(_) => Err(format!(
                "{modifier} can't be held, so it can't be a modifier"
            )),
            _ if modifier.kind() != SourceKind::Digital => Err(format!(
                "{modifier} isn't a button, so it can't be a modifier"
            )),
            _ => resolve(modifier, slots),
        })
        .collect::<Result<Vec<_>, _>>()?;

    let kind = binding.source.kind();
    let role = match (action.action().kind, kind) {
        (ActionKind::Hold, SourceKind::Digital) => Role::Button {
            toggle: binding.toggle,
        },
        (ActionKind::OneShot, SourceKind::Digital) => Role::Button { toggle: false },
        (ActionKind::Hold | ActionKind::OneShot, _) => {
            return Err(format!(
                "{} is a button action, so an axis needs a threshold to drive it",
                binding.action
            ));
        }
        (ActionKind::Axis(axis), SourceKind::Analog) => match binding.tuning.mode {
            AxisMode::Absolute => Role::AbsoluteAxis,
            AxisMode::Relative
                if axis.behaviour == (AxisBehaviour::Position { relative: true }) =>
            {
                Role::RelativeAxis
            }
            AxisMode::Relative => {
                return Err(format!(
                    "{} can't be driven in relative mode",
                    binding.action
                ));
            }
        },
        (ActionKind::Axis(_), SourceKind::Delta) => Role::MouseAxis,
        (ActionKind::Axis(axis), SourceKind::Digital) if binding.set_to.is_some() => {
            if axis.behaviour == AxisBehaviour::Rate {
                return Err(format!(
                    "{} is a speed, so a button can't set where it is",
                    binding.action
                ));
            }
            Role::AxisSetter {
                position: binding.set_to.unwrap_or_default().clamp(axis.min, 1.0),
            }
        }
        (ActionKind::Axis(_), SourceKind::Digital) => {
            let direction = binding
                .direction
                .ok_or_else(|| "a button on an axis needs a direction".to_owned())?;
            let wheel_step = matches!(binding.source, Source::Wheel(_))
                .then_some(WHEEL_STEP * binding.tuning.scale);
            Role::AxisButton {
                sign: direction.sign(),
                step: binding.step.or(wheel_step),
            }
        }
    };

    Ok(Compiled {
        action,
        trigger,
        modifiers,
        outranked_by: Vec::new(),
        role,
        tuning: binding.tuning.clone(),
        counter: None,
        was_active: true,
        toggled: false,
        waiting: None,
    })
}
