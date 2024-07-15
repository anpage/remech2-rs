use anyhow::Result;
use retour::{Function, GenericDetour, HookableWith};

pub unsafe fn hook_function<T, D>(address: T, detour: D) -> Result<GenericDetour<T>>
where
    T: HookableWith<D>,
    D: Function,
{
    let detour = GenericDetour::new(address, detour)?;
    detour.enable()?;
    Ok(detour)
}
