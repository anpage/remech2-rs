/// Declares game globals: places in the linked game code, like `shell::g_soundConfig.m_midiVolume`.
#[macro_export]
macro_rules! globals {
    () => {};
    (
        $(#[$attr:meta])*
        $vis:vis static $name:ident: $ty:ty = $place:expr;
        $($rest:tt)*
    ) => {
        $(#[$attr])*
        $vis static $name: $crate::global::Global<$ty> =
            $crate::global::Global::linked(unsafe { &raw mut $place });
        $crate::globals!($($rest)*);
    };
}
pub use crate::globals;
