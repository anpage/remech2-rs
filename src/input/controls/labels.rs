use crate::input::{
    KeyCode,
    binding::Binding,
    kbm::{MouseButton, WheelDirection},
    pad::HatDirection,
    source::{Modifier, Source, Threshold},
};

pub fn key(key: KeyCode) -> String {
    use KeyCode::*;
    let name = key.name();
    let fixed = match key {
        Backquote => "`",
        Backslash => "\\",
        BracketLeft => "[",
        BracketRight => "]",
        Comma => ",",
        Equal => "=",
        Minus => "-",
        Period => ".",
        Quote => "'",
        Semicolon => ";",
        Slash => "/",
        IntlBackslash => "\\ (ISO)",
        ShiftLeft => "Left Shift",
        ShiftRight => "Right Shift",
        ControlLeft => "Left Ctrl",
        ControlRight => "Right Ctrl",
        AltLeft => "Left Alt",
        AltRight => "Right Alt",
        MetaLeft => "Left Meta",
        MetaRight => "Right Meta",
        ArrowUp => "Up",
        ArrowDown => "Down",
        ArrowLeft => "Left",
        ArrowRight => "Right",
        ContextMenu => "Menu",
        NumpadAdd => "Num +",
        NumpadSubtract => "Num -",
        NumpadMultiply => "Num *",
        NumpadDivide => "Num /",
        NumpadDecimal => "Num .",
        NumpadComma => "Num ,",
        NumpadEqual => "Num =",
        NumpadEnter => "Num Enter",
        _ => "",
    };
    if !fixed.is_empty() {
        return fixed.to_owned();
    }
    if let Some(rest) = name
        .strip_prefix("Key")
        .or_else(|| name.strip_prefix("Digit"))
    {
        return rest.to_owned();
    }
    if let Some(digit) = name.strip_prefix("Numpad") {
        return format!("Num {digit}");
    }
    split_words(name)
}

fn split_words(name: &str) -> String {
    let mut words = String::new();
    let mut previous: Option<char> = None;
    for c in name.chars() {
        // Not F12, which is one word
        let new_word = (c.is_ascii_uppercase() || c.is_ascii_digit())
            && previous.is_some_and(|p| p.is_ascii_lowercase());
        if new_word {
            words.push(' ');
        }
        words.push(c);
        previous = Some(c);
    }
    words
}

pub fn modifier(modifier: Modifier) -> &'static str {
    match modifier {
        Modifier::Shift => "Shift",
        Modifier::Control => "Ctrl",
        Modifier::Alt => "Alt",
        Modifier::Meta => "Meta",
    }
}

pub fn mouse_button(button: MouseButton) -> String {
    format!("Mouse {}", button.name())
}

pub fn wheel(direction: WheelDirection) -> String {
    format!("Wheel {}", direction.name().to_lowercase())
}

pub fn hat_direction(direction: HatDirection) -> &'static str {
    match direction {
        HatDirection::Up => "up",
        HatDirection::UpRight => "up-right",
        HatDirection::Right => "right",
        HatDirection::DownRight => "down-right",
        HatDirection::Down => "down",
        HatDirection::DownLeft => "down-left",
        HatDirection::Left => "left",
        HatDirection::UpLeft => "up-left",
    }
}

pub fn source(source: &Source, device: &impl Fn(&str) -> String) -> String {
    match source {
        Source::Key(k) => key(*k),
        Source::Modifier(m) => modifier(*m).to_owned(),
        Source::MouseButton(button) => mouse_button(*button),
        Source::Wheel(direction) => wheel(*direction),
        Source::MouseAxis(axis) => format!("Mouse {}", axis.name()),
        Source::Button { device: id, index } => {
            format!("{}: Button {}", device(id), index + 1)
        }
        Source::Hat {
            device: id,
            index,
            direction,
        } => format!(
            "{}: Hat {} {}",
            device(id),
            index + 1,
            hat_direction(*direction)
        ),
        Source::Axis { device: id, axis } => format!("{}: {axis} axis", device(id)),
        Source::AxisThreshold {
            device: id,
            axis,
            threshold,
        } => {
            let (side, value) = match threshold {
                Threshold::Above(value) => ("above", value),
                Threshold::Below(value) => ("below", value),
            };
            format!("{}: {axis} {side} {:.0}%", device(id), value * 100.0)
        }
    }
}

pub fn chord(binding: &Binding, device: &impl Fn(&str) -> String) -> String {
    binding
        .modifiers
        .iter()
        .chain([&binding.source])
        .map(|part| source(part, device))
        .collect::<Vec<_>>()
        .join(" + ")
}
