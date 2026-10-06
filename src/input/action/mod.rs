use std::{collections::HashMap, sync::LazyLock};

use list::ACTIONS;

pub mod list;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub enum Category {
    Movement,
    Torso,
    JumpJets,
    Weapons,
    Targeting,
    Navigation,
    MechSystems,
    Displays,
    Views,
    Camera,
    Lance,
    Game,
    SimMenu,
    Debug,
}

impl Category {
    pub fn label(self) -> &'static str {
        match self {
            Category::Movement => "MOVEMENT",
            Category::Torso => "TORSO",
            Category::JumpJets => "JUMP JETS",
            Category::Weapons => "WEAPONS",
            Category::Targeting => "TARGETING",
            Category::Navigation => "NAVIGATION",
            Category::MechSystems => "MECH SYSTEMS",
            Category::Displays => "DISPLAYS",
            Category::Views => "VIEWS",
            Category::Camera => "EXTERNAL CAMERA",
            Category::Lance => "LANCE",
            Category::Game => "GAME",
            Category::SimMenu => "IN-MISSION MENU",
            Category::Debug => "DEBUG",
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Context {
    /// The game is unpaused and the player is controlling their mech
    pub gameplay: bool,
    /// A menu is open that takes input
    pub menu: bool,
}

impl Context {
    /// A menu that leaves the controls live, like the lance command menus
    pub const GAMEPLAY_AND_MENU: Self = Self {
        gameplay: true,
        menu: true,
    };
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Availability {
    Gameplay,
    Menu,
    Always,
}

impl Availability {
    pub fn includes(self, context: Context) -> bool {
        match self {
            Availability::Gameplay => context.gameplay,
            Availability::Menu => context.menu,
            Availability::Always => true,
        }
    }

    pub fn clashes_with(self, other: Availability) -> bool {
        !matches!(
            (self, other),
            (Availability::Gameplay, Availability::Menu)
                | (Availability::Menu, Availability::Gameplay)
        )
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum ActionKind {
    /// On for as long as a binding is held
    Hold,
    /// Fires once per press
    OneShot,
    Axis(AxisAction),
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum AxisBehaviour {
    /// Holds where it's put, like torso twist or throttle.
    /// `relative` says whether a stick may drive it by rate, not just by position.
    Position { relative: bool },
    /// A speed, back to zero when let go, like turning the legs
    Rate,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct AxisAction {
    pub behaviour: AxisBehaviour,
    /// The sim's own increase and decrease sinks, which our button ramp replaces
    pub ramp_sinks: Option<[&'static str; 2]>,
    /// How fast button inputs speed up, in travel per second squared
    pub ramp_acceleration: f64,
    /// A button action that resets the axis to its rest position
    pub reset: Option<&'static str>,
    pub set_sink: Option<&'static str>,
    pub min: f64,
    pub rest: f64,
    /// Labels for increasing and decreasing the axis
    pub directions: [&'static str; 2],
}

impl AxisAction {
    fn side_span(&self, value: f64) -> f64 {
        if value >= 0.0 {
            1.0 - self.rest
        } else {
            1.0 + self.rest
        }
    }

    pub fn sink_position(&self, value: f64) -> f64 {
        self.rest + value * self.side_span(value)
    }

    pub fn value_at(&self, position: f64) -> f64 {
        let offset = position - self.rest;
        let span = self.side_span(offset);
        if span > 0.0 { offset / span } else { 0.0 }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Target {
    /// An entry in the sim's input sink table
    Sink(&'static str),
    GameKey(u8),
    /// A key code for the pause menu
    MenuKey(i16),
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Action {
    /// The stable name used to identify this action in profiles and bindings
    pub name: &'static str,
    pub label: &'static str,
    pub category: Category,
    pub kind: ActionKind,
    pub target: Target,
    pub availability: Availability,
    pub also: Option<&'static str>,
}

/// Derived from the fact that the original game's fuel recharge broke below 4 ticks per frame.
/// 45.5 FPS was the highest you could go before things started to break.
const IDEAL_FRAMES_PER_SECOND: f64 = 182.0 / 4.0;

/// Sim axis positions run from -65536 to 65536
const SIM_AXIS_UNITS: f64 = 65536.0;

/// The rate at which buttons will accelerate along their axis.
/// The original game didn't account for framerate, so we tie the rate to the assumed "ideal" 45.5 FPS.
const fn ramp_acceleration(shift: u32) -> f64 {
    (12u32 << shift) as f64 * IDEAL_FRAMES_PER_SECOND * IDEAL_FRAMES_PER_SECOND / SIM_AXIS_UNITS
}

/// The sim caps a key ramp at 32768 units per frame.
/// Again, we tie this to the assumed "ideal" 45.5 FPS.
pub const RAMP_SPEED_MAX: f64 = 32768.0 * IDEAL_FRAMES_PER_SECOND / SIM_AXIS_UNITS;

const fn action(
    name: &'static str,
    label: &'static str,
    category: Category,
    kind: ActionKind,
    target: Target,
) -> Action {
    Action {
        name,
        label,
        category,
        kind,
        target,
        availability: if matches!(category, Category::SimMenu) {
            Availability::Menu
        } else {
            Availability::Gameplay
        },
        also: None,
    }
}

const fn hold(
    name: &'static str,
    label: &'static str,
    category: Category,
    sink: &'static str,
) -> Action {
    action(name, label, category, ActionKind::Hold, Target::Sink(sink))
}

const fn press(
    name: &'static str,
    label: &'static str,
    category: Category,
    sink: &'static str,
) -> Action {
    action(
        name,
        label,
        category,
        ActionKind::OneShot,
        Target::Sink(sink),
    )
}

const fn key(name: &'static str, label: &'static str, category: Category, code: u8) -> Action {
    action(
        name,
        label,
        category,
        ActionKind::OneShot,
        Target::GameKey(code),
    )
}

const fn menu_key(name: &'static str, label: &'static str, code: i16) -> Action {
    action(
        name,
        label,
        Category::SimMenu,
        ActionKind::OneShot,
        Target::MenuKey(code),
    )
}

const fn debug(name: &'static str, label: &'static str, code: u8) -> Action {
    key(name, label, Category::Debug, code)
}

#[allow(clippy::too_many_arguments)]
const fn axis(
    name: &'static str,
    label: &'static str,
    category: Category,
    sink: &'static str,
    behaviour: AxisBehaviour,
    ramp_sinks: Option<[&'static str; 2]>,
    shift: u32,
    reset: Option<&'static str>,
) -> Action {
    let axis = AxisAction {
        behaviour,
        ramp_sinks,
        ramp_acceleration: ramp_acceleration(shift),
        reset,
        set_sink: None,
        min: -1.0,
        rest: 0.0,
        directions: ["increase", "decrease"],
    };
    action(
        name,
        label,
        category,
        ActionKind::Axis(axis),
        Target::Sink(sink),
    )
}

impl Action {
    const fn always(self) -> Self {
        Action {
            availability: Availability::Always,
            ..self
        }
    }

    const fn set_by(self, sink: &'static str) -> Self {
        let ActionKind::Axis(mut axis) = self.kind else {
            panic!("only axes can be set");
        };
        axis.set_sink = Some(sink);
        Action {
            kind: ActionKind::Axis(axis),
            ..self
        }
    }

    const fn rests_off_center(self, rest: i32, min: i32, max: i32) -> Self {
        let ActionKind::Axis(mut axis) = self.kind else {
            panic!("only axes have a range");
        };
        axis.rest = (rest - min) as f64 * 2.0 / (max - min) as f64 - 1.0;
        if rest == min {
            axis.min = 0.0;
        }
        Action {
            kind: ActionKind::Axis(axis),
            ..self
        }
    }

    const fn stops_at_rest(self) -> Self {
        let ActionKind::Axis(mut axis) = self.kind else {
            panic!("only axes have a range");
        };
        axis.min = 0.0;
        Action {
            kind: ActionKind::Axis(axis),
            ..self
        }
    }

    const fn directions(self, increase: &'static str, decrease: &'static str) -> Self {
        let ActionKind::Axis(mut axis) = self.kind else {
            panic!("only axes have directions");
        };
        axis.directions = [increase, decrease];
        Action {
            kind: ActionKind::Axis(axis),
            ..self
        }
    }

    const fn also(self, sink: &'static str) -> Self {
        Action {
            also: Some(sink),
            ..self
        }
    }

    pub fn axis(&self) -> Option<&AxisAction> {
        match &self.kind {
            ActionKind::Axis(axis) => Some(axis),
            _ => None,
        }
    }
}

use AxisBehaviour::{Position, Rate};
use Category::*;

/// An index into `ACTIONS`
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct ActionId(u16);

static BY_NAME: LazyLock<HashMap<&'static str, ActionId>> = LazyLock::new(|| {
    ACTIONS
        .iter()
        .enumerate()
        .map(|(index, action)| (action.name, ActionId(index as u16)))
        .collect()
});

impl ActionId {
    pub fn find(name: &str) -> Option<Self> {
        BY_NAME.get(name).copied()
    }

    pub fn all() -> impl Iterator<Item = ActionId> {
        (0..ACTIONS.len()).map(|index| ActionId(index as u16))
    }

    pub fn index(self) -> usize {
        self.0 as usize
    }

    pub fn action(self) -> &'static Action {
        &ACTIONS[self.index()]
    }
}
