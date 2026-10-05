use std::collections::BTreeMap;

use super::{
    pad::Device,
    profile::{DeviceSlot, UsbId},
};

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Assignment {
    by_slot: BTreeMap<String, usize>,
}

impl Assignment {
    pub fn device(&self, slot: &str) -> Option<usize> {
        self.by_slot.get(slot).copied()
    }

    pub fn slot(&self, device: usize) -> Option<&str> {
        self.by_slot
            .iter()
            .find(|&(_, &id)| id == device)
            .map(|(slot, _)| slot.as_str())
    }

    pub fn assign(&mut self, slot: &str, device: usize) {
        self.by_slot.retain(|_, id| *id != device);
        self.by_slot.insert(slot.to_owned(), device);
    }

    pub fn unassign(&mut self, slot: &str) {
        self.by_slot.remove(slot);
    }

    pub fn iter(&self) -> impl Iterator<Item = (&str, usize)> {
        self.by_slot.iter().map(|(slot, &id)| (slot.as_str(), id))
    }
}

pub fn match_slots(slots: &[DeviceSlot], devices: &[Device], previous: &Assignment) -> Assignment {
    let connected: Vec<&Device> = devices.iter().filter(|device| device.connected).collect();
    let mut assignment = Assignment::default();
    for (slot, id) in previous.iter() {
        let slot_exists = slots.iter().any(|s| s.id == slot);
        let still_connected = connected.iter().any(|device| device.id == id);
        if slot_exists && still_connected {
            assignment.by_slot.insert(slot.to_owned(), id);
        }
    }

    let serial = |usb_id: Option<UsbId>, serial: Option<&String>| {
        Some((usb_id?, serial.filter(|serial| !serial.is_empty())?.clone()))
    };
    match_unique(
        slots,
        &connected,
        &mut assignment,
        |slot| serial(slot.usb_id, slot.serial_number.as_ref()),
        |device| serial(UsbId::of(device), device.serial_number.as_ref()),
    );

    match_unique(
        slots,
        &connected,
        &mut assignment,
        |slot| slot.persistent_id.clone(),
        |device| device.persistent_id.clone(),
    );

    match_unique(
        slots,
        &connected,
        &mut assignment,
        |slot| slot.usb_id,
        UsbId::of,
    );

    assignment
}

fn match_unique<K: PartialEq>(
    slots: &[DeviceSlot],
    devices: &[&Device],
    assignment: &mut Assignment,
    slot_key: impl Fn(&DeviceSlot) -> Option<K>,
    device_key: impl Fn(&Device) -> Option<K>,
) {
    for slot in slots {
        if assignment.device(&slot.id).is_some() {
            continue;
        }

        let Some(key) = slot_key(slot) else {
            continue;
        };

        let rival_slots = slots
            .iter()
            .filter(|other| other.id != slot.id && assignment.device(&other.id).is_none())
            .filter(|other| slot_key(other).as_ref() == Some(&key))
            .count();
        if rival_slots > 0 {
            continue;
        }
        let mut candidates = devices
            .iter()
            .filter(|device| assignment.slot(device.id).is_none())
            .filter(|device| device_key(device).as_ref() == Some(&key));
        if let (Some(device), None) = (candidates.next(), candidates.next()) {
            assignment.by_slot.insert(slot.id.clone(), device.id);
        }
    }
}
