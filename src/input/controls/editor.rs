use std::collections::HashMap;

use anyhow::Result;

use super::capture;
use crate::input::{
    KeyCode,
    action::{ActionId, Context},
    binding::Binding,
    eval::{ActionState, Evaluator, InputFrame, Problem},
    kbm::KbmState,
    matching::{Assignment, match_slots},
    pad::Device,
    profile::{DeviceSlot, Profile},
    source::{Modifier, Source},
    store::{self, Store},
};

pub struct Editor {
    store: Store,
    name: String,
    profile: Profile,
    saved: Profile,
    draft: bool,
    active: String,
    assignment: Assignment,
    evaluator: Evaluator,
    problems: Vec<Problem>,
    shared: HashMap<String, Vec<String>>,
    pub save_error: Option<String>,
}

/// The default name for a new custom profile
const CUSTOM_PROFILE: &str = "Custom";

fn chord_key(binding: &Binding) -> String {
    let mut modifiers: Vec<String> = binding.modifiers.iter().map(Source::to_string).collect();
    modifiers.sort();
    format!("{}|{}", binding.source, modifiers.join("|"))
}

impl Editor {
    pub fn open(store: Store, active: &str) -> Self {
        let (name, profile) = store.load_or_default(active);
        let (evaluator, problems) = Evaluator::new(&profile);
        let mut editor = Self {
            store,
            active: name.clone(),
            name,
            saved: profile.clone(),
            profile,
            draft: false,
            assignment: Assignment::default(),
            evaluator,
            problems,
            shared: HashMap::new(),
            save_error: None,
        };
        editor.find_shared();
        editor
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn active(&self) -> &str {
        &self.active
    }

    pub fn is_default(&self) -> bool {
        !self.draft && store::is_default(&self.name)
    }

    pub fn profile(&self) -> &Profile {
        &self.profile
    }

    pub fn store(&self) -> &Store {
        &self.store
    }

    pub fn assignment(&self) -> &Assignment {
        &self.assignment
    }

    pub fn problem(&self, binding: usize) -> Option<&str> {
        self.problems
            .iter()
            .find(|problem| problem.binding == binding)
            .map(|problem| problem.message.as_str())
    }

    pub fn shared_with(&self, binding: usize) -> Vec<&'static str> {
        let Some(binding) = self.profile.bindings.get(binding) else {
            return Vec::new();
        };
        let action = |name: &str| ActionId::find(name).map(ActionId::action);
        let availability = action(&binding.action).map(|action| action.availability);
        self.shared
            .get(&chord_key(binding))
            .into_iter()
            .flatten()
            .filter(|other| **other != binding.action)
            .map(|other| action(other))
            .filter(|other| {
                let theirs = other.map(|other| other.availability);
                availability
                    .zip(theirs)
                    .is_none_or(|(ours, theirs)| ours.clashes_with(theirs))
            })
            .map(|other| other.map_or("an unknown action", |other| other.label))
            .collect()
    }

    pub fn edited(&self) -> bool {
        self.draft || self.profile != self.saved
    }

    pub fn has_changes(&self) -> bool {
        self.edited() || self.name != self.active
    }

    pub fn edit<T>(&mut self, change: impl FnOnce(&mut Profile) -> T) -> T {
        let before = self.is_default().then(|| self.profile.clone());
        let result = change(&mut self.profile);
        if before.is_some_and(|before| before != self.profile) {
            self.name = self.store.unused_name(CUSTOM_PROFILE);
            self.draft = true;
        }
        self.changed();
        result
    }

    fn changed(&mut self) {
        (self.evaluator, self.problems) = Evaluator::new(&self.profile);
        self.find_shared();
    }

    pub fn apply(&mut self) -> Result<()> {
        let saved = if self.is_default() {
            Ok(())
        } else {
            self.store.save_profile(&self.name, &self.profile)
        };
        self.save_error = saved.as_ref().err().map(|e| format!("{e:#}"));
        saved?;
        self.saved = self.profile.clone();
        self.draft = false;
        self.active = self.name.clone();
        Ok(())
    }

    fn find_shared(&mut self) {
        let mut by_chord: HashMap<String, Vec<String>> = HashMap::new();
        for binding in &self.profile.bindings {
            let actions = by_chord.entry(chord_key(binding)).or_default();
            if !actions.contains(&binding.action) {
                actions.push(binding.action.clone());
            }
        }
        by_chord.retain(|_, actions| actions.len() > 1);
        self.shared = by_chord;
    }

    pub fn match_devices(&mut self, devices: &[Device]) {
        self.assignment = match_slots(&self.profile.devices, devices, &self.assignment);
    }

    pub fn evaluate(&mut self, kbm: &KbmState, devices: &[Device], seconds: f64) -> &ActionState {
        let frame = InputFrame {
            kbm,
            devices,
            assignment: &self.assignment,
            typing: false,
        };
        self.evaluator
            .update(&frame, seconds, Context::GAMEPLAY_AND_MENU)
    }

    pub fn state(&self) -> &ActionState {
        self.evaluator.state()
    }

    pub fn slot_for(&mut self, device: &Device) -> String {
        if let Some(slot) = self.assignment.slot(device.id) {
            return slot.to_owned();
        }

        let id = self.profile.unused_slot_id("device");
        let name = (1..)
            .map(|n| match n {
                1 => device.name.clone(),
                n => format!("{} {n}", device.name),
            })
            .find(|name| self.profile.devices.iter().all(|slot| &slot.name != name))
            .unwrap();
        let slot = DeviceSlot::new(id.clone(), name, device);

        self.edit(|profile| profile.devices.push(slot));
        self.assignment.assign(&id, device.id);
        id
    }

    pub fn claim(&mut self, slot: &str, device: &Device) {
        self.assignment.assign(slot, device.id);
        self.edit(|profile| {
            if let Some(slot) = profile.devices.iter_mut().find(|s| s.id == slot) {
                slot.record_identity(device);
            }
        });
    }

    pub fn remove_slot(&mut self, slot: &str) {
        self.assignment.unassign(slot);
        self.edit(|profile| {
            profile.devices.retain(|s| s.id != slot);
            profile.bindings.retain(|binding| {
                binding
                    .sources()
                    .all(|source| source.device() != Some(slot))
            });
        });
    }

    pub fn bindings_on(&self, slot: &str) -> usize {
        self.profile
            .bindings
            .iter()
            .filter(|binding| {
                binding
                    .sources()
                    .any(|source| source.device() == Some(slot))
            })
            .count()
    }

    pub fn source_for(&mut self, input: &capture::Input, devices: &[Device]) -> Option<Source> {
        let mut slot = |id: usize| {
            let device = devices.iter().find(|device| device.id == id)?;
            Some(self.slot_for(device))
        };

        Some(match *input {
            capture::Input::Key(key) => Source::Key(key),
            capture::Input::MouseButton(button) => Source::MouseButton(button),
            capture::Input::Wheel(direction) => Source::Wheel(direction),
            capture::Input::Button { device, index } => Source::Button {
                device: slot(device)?,
                index,
            },
            capture::Input::Hat {
                device,
                index,
                direction,
            } => Source::Hat {
                device: slot(device)?,
                index,
                direction,
            },
            capture::Input::Axis { device, axis } => Source::Axis {
                device: slot(device)?,
                axis,
            },
            capture::Input::Threshold {
                device,
                axis,
                threshold,
            } => Source::AxisThreshold {
                device: slot(device)?,
                axis,
                threshold,
            },
        })
    }

    pub fn binding_for(
        &mut self,
        action: &str,
        captured: &capture::Captured,
        devices: &[Device],
    ) -> Option<Binding> {
        let mut binding = Binding::new(action, self.source_for(&captured.input, devices)?);
        for modifier in &captured.modifiers {
            let source = match modifier {
                capture::Input::Key(key) => match modifier_of(*key) {
                    Some(modifier) => Source::Modifier(modifier),
                    None => Source::Key(*key),
                },
                _ => self.source_for(modifier, devices)?,
            };
            if !binding.modifiers.contains(&source) {
                binding.modifiers.push(source);
            }
        }
        Some(binding)
    }

    pub fn add_binding(&mut self, binding: Binding) -> usize {
        self.edit(|profile| {
            profile.bindings.push(binding);
            profile.bindings.len() - 1
        })
    }

    pub fn remove_binding(&mut self, index: usize) {
        self.edit(|profile| {
            if index < profile.bindings.len() {
                profile.bindings.remove(index);
            }
        });
    }

    pub fn switch(&mut self, name: &str) {
        let (name, profile) = self.store.load_or_default(name);
        self.name = name;
        self.saved = profile.clone();
        self.profile = profile;
        self.draft = false;
        self.save_error = None;
        self.changed();
    }

    pub fn create(&mut self, name: &str, profile: &Profile) -> Result<()> {
        if self.store.has_profile(name) {
            anyhow::bail!("There's already a profile called {name:?}");
        }
        self.name = name.to_owned();
        self.profile = profile.clone();
        self.draft = true;
        self.changed();
        Ok(())
    }

    pub fn rename(&mut self, name: &str) -> Result<()> {
        if !self.draft {
            self.store.rename_profile(&self.name, name)?;
            if self.active == self.name {
                self.active = name.to_owned();
            }
        } else if self.store.has_profile(name) && !name.eq_ignore_ascii_case(&self.name) {
            anyhow::bail!("There's already a profile called {name:?}");
        }
        self.name = name.to_owned();
        Ok(())
    }

    pub fn delete(&mut self) -> Result<()> {
        self.store.delete_profile(&self.name)?;
        if self.active == self.name {
            self.active = store::DEFAULT_PROFILE.to_owned();
        }
        self.switch(store::DEFAULT_PROFILE);
        Ok(())
    }
}

fn modifier_of(key: KeyCode) -> Option<Modifier> {
    Modifier::ALL
        .into_iter()
        .find(|modifier| modifier.keys().contains(&key))
}
