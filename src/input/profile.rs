use std::{fmt, str::FromStr};

use anyhow::{Context as _, Result, bail};
use serde::{Deserialize, Deserializer, Serialize, Serializer, de};

use super::{binding::Binding, pad::Device, source::valid_device_id};

/// Bump when a change needs migrating and migrate on load
pub const FORMAT_VERSION: u32 = 1;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct UsbId {
    pub vendor: u16,
    pub product: u16,
}

impl UsbId {
    pub fn of(device: &Device) -> Option<Self> {
        Some(Self {
            vendor: device.vendor_id?,
            product: device.product_id?,
        })
    }
}

impl fmt::Display for UsbId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:04x}:{:04x}", self.vendor, self.product)
    }
}

impl FromStr for UsbId {
    type Err = String;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let bad = || format!("expected a USB ID like 044f:b10a, got {s:?}");
        let (vendor, product) = s.split_once(':').ok_or_else(bad)?;
        Ok(Self {
            vendor: u16::from_str_radix(vendor, 16).map_err(|_| bad())?,
            product: u16::from_str_radix(product, 16).map_err(|_| bad())?,
        })
    }
}

impl Serialize for UsbId {
    fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.collect_str(self)
    }
}

impl<'de> Deserialize<'de> for UsbId {
    fn deserialize<D: Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        String::deserialize(deserializer)?
            .parse()
            .map_err(de::Error::custom)
    }
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct DeviceSlot {
    pub id: String,
    pub name: String,
    #[serde(default)]
    pub product: String,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub usb_id: Option<UsbId>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub serial_number: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub persistent_id: Option<String>,
    #[serde(default)]
    pub axes: usize,
    #[serde(default)]
    pub buttons: usize,
    #[serde(default)]
    pub hats: usize,
}

impl DeviceSlot {
    pub fn new(id: String, name: String, device: &Device) -> Self {
        let mut slot = Self {
            id,
            name,
            product: String::new(),
            usb_id: None,
            serial_number: None,
            persistent_id: None,
            axes: 0,
            buttons: 0,
            hats: 0,
        };
        slot.record_identity(device);
        slot
    }

    pub fn record_identity(&mut self, device: &Device) {
        self.product = device.name.clone();
        self.usb_id = UsbId::of(device);
        self.serial_number = device.serial_number.clone();
        self.persistent_id = device.persistent_id.clone();
        self.axes = device.axes.len();
        self.buttons = device.buttons.len();
        self.hats = device.hats.len();
    }
}

#[derive(Clone, Debug, Default, PartialEq)]
pub struct Profile {
    pub devices: Vec<DeviceSlot>,
    pub bindings: Vec<Binding>,
    pub unreadable: Vec<toml::Table>,
}

#[derive(Serialize, Deserialize)]
struct ProfileFile {
    format: u32,
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    devices: Vec<DeviceSlot>,
    #[serde(default)]
    bindings: Vec<toml::Table>,
}

impl Profile {
    pub fn from_toml(text: &str) -> Result<(Self, Vec<String>)> {
        let file: ProfileFile = toml::from_str(text)?;
        let mut warnings = check_format(file.format);
        let (bindings, unreadable) = read_bindings(file.bindings, &mut warnings);

        let mut ids = Vec::new();
        for slot in &file.devices {
            if !valid_device_id(&slot.id) {
                warnings.push(format!("Device slot ID {:?} isn't valid", slot.id));
            } else if ids.contains(&&slot.id) {
                warnings.push(format!("Device slot ID {:?} is used twice", slot.id));
            }
            ids.push(&slot.id);
        }

        let profile = Self {
            devices: file.devices,
            bindings,
            unreadable,
        };
        Ok((profile, warnings))
    }

    pub fn to_toml(&self) -> Result<String> {
        let file = ProfileFile {
            format: FORMAT_VERSION,
            devices: self.devices.clone(),
            bindings: write_bindings(&self.bindings, &self.unreadable)?,
        };
        Ok(toml::to_string_pretty(&file)?)
    }

    pub fn slot(&self, id: &str) -> Option<&DeviceSlot> {
        self.devices.iter().find(|slot| slot.id == id)
    }

    pub fn unused_slot_id(&self, base: &str) -> String {
        (1..)
            .map(|n| format!("{base}{n}"))
            .find(|id| self.slot(id).is_none())
            .unwrap()
    }
}

fn check_format(format: u32) -> Vec<String> {
    if format > FORMAT_VERSION {
        vec![format!(
            "Written by a newer version (format {format}, we read {FORMAT_VERSION}). \
             Some settings may be ignored"
        )]
    } else {
        Vec::new()
    }
}

fn read_bindings(
    tables: Vec<toml::Table>,
    warnings: &mut Vec<String>,
) -> (Vec<Binding>, Vec<toml::Table>) {
    let mut bindings = Vec::new();
    let mut unreadable = Vec::new();
    for (index, table) in tables.into_iter().enumerate() {
        match toml::Value::Table(table.clone()).try_into::<Binding>() {
            Ok(binding) => bindings.push(binding),
            Err(e) => {
                warnings.push(format!(
                    "Binding {} can't be read: {}",
                    index + 1,
                    e.message()
                ));
                unreadable.push(table);
            }
        }
    }
    (bindings, unreadable)
}

fn write_bindings(bindings: &[Binding], unreadable: &[toml::Table]) -> Result<Vec<toml::Table>> {
    let mut tables = Vec::with_capacity(bindings.len() + unreadable.len());
    for binding in bindings {
        match toml::Value::try_from(binding).context("Couldn't write a binding")? {
            toml::Value::Table(table) => tables.push(table),
            _ => bail!("A binding didn't turn into a table"),
        }
    }
    tables.extend(unreadable.iter().cloned());
    Ok(tables)
}
