/// Declares game globals against the file's `MODULE`.
#[macro_export]
macro_rules! globals {
    ($(
        $(#[$attr:meta])*
        $vis:vis static $name:ident: $ty:ty = $rva:literal;
    )*) => {$(
        $(#[$attr])*
        $vis static $name: $crate::global::Global<$ty> =
            $crate::global::Global::new(&MODULE, $rva);
    )*};
}
pub use crate::globals;

#[macro_export]
macro_rules! game_fns {
    ($(
        $(#[$attr:meta])*
        $vis:vis static $name:ident: $sig:ty = $rva:literal;
    )*) => {$(
        $(#[$attr])*
        $vis static $name: $crate::game_fn::GameFn<$sig> =
            $crate::game_fn::GameFn::new(&MODULE, $rva);
    )*};
}
pub use crate::game_fns;

/// Declares a detour over a game function, and its `RawDetour` counterpart.
///
/// Both are attribute macros so that rustfmt formats the hook bodies; see
/// `binding-macros/src/lib.rs`. They're re-exported here so that every binding
/// macro comes from one place.
pub use binding_macros::{hook, raw_hook};

/// Points an address at a detour that's defined elsewhere. For detours [`hook!`]
/// can't express: variadics, which can't implement `Function`, and detours whose
/// signature differs from the original's. The latter reach the original through
/// `RawHook::original`, naming the true signature at the call site.
#[macro_export]
macro_rules! raw_detour {
    ($(
        $(#[$attr:meta])*
        $vis:vis static $name:ident at $rva:literal $(in $module:ident)? => $detour:path;
    )*) => {$(
        $(#[$attr])*
        $vis static $name: $crate::hook::RawHook = $crate::hook::RawHook::new(
            &$crate::pick_module!($($module)?),
            $rva,
            stringify!($name),
            $detour as *const (),
        );
    )*};
}
pub use crate::raw_detour;

/// `MODULE` unless an override was given. Keeps the `in OTHER` suffix from doubling every rule.
#[doc(hidden)]
#[macro_export]
macro_rules! pick_module {
    () => {
        MODULE
    };
    ($module:ident) => {
        $module
    };
}
pub use crate::pick_module;

/// Declares a table of import thunks to overwrite.
#[macro_export]
macro_rules! thunks {
    (
        $(#[$attr:meta])*
        $vis:vis static $name:ident $(in $module:ident)? = [
            $($rva:literal => $replacement:path),* $(,)?
        ];
    ) => {
        $(#[$attr])*
        $vis static $name: $crate::patch::Table<$crate::thunk::Thunk> =
            $crate::patch::Table::new(
                stringify!($name),
                &[$($crate::thunk::Thunk::new(
                    &$crate::pick_module!($($module)?),
                    $rva,
                    stringify!($replacement),
                    $replacement as *const (),
                )),*],
            );
    };
}
pub use crate::thunks;

/// Declares a table of raw byte writes into the game's data.
#[macro_export]
macro_rules! data_patches {
    (
        $(#[$attr:meta])*
        $vis:vis static $name:ident $(in $module:ident)? = [
            $($label:ident: $rva:literal => $bytes:expr),* $(,)?
        ];
    ) => {
        $(#[$attr])*
        $vis static $name: $crate::patch::Table<$crate::thunk::DataPatch> =
            $crate::patch::Table::new(
                stringify!($name),
                &[$($crate::thunk::DataPatch::new(
                    &$crate::pick_module!($($module)?),
                    $rva,
                    stringify!($label),
                    $bytes,
                )),*],
            );
    };
}
pub use crate::data_patches;

/// Collects a file's bindings into the list its parent module registers.
///
/// `hook <name>` takes anything declared with [`hook!`] or [`raw_hook!`];
/// `patch <name>` takes a named static, such as a thunk table, a data patch
/// table or a [`raw_detour!`].
#[macro_export]
macro_rules! patches {
    (
        $(#[$attr:meta])*
        $vis:vis static $name:ident = [$($kind:ident $item:ident),* $(,)?];
    ) => {
        $($crate::registration!($kind $item);)*

        /// Marker `patch_groups!` implements `Grouped` for.
        $vis struct PatchGroup;

        const _: () = {
            const fn assert_grouped<T: $crate::patch::Grouped>() {}
            assert_grouped::<PatchGroup>();
        };

        $(#[$attr])*
        $vis static $name: &[&'static dyn $crate::patch::Patch] =
            &[$($crate::patch_ref!($kind $item)),*];
    };
}
pub use crate::patches;

/// Collects the modules whose [`patches!`] lists a parent module applies.
///
/// Naming a module here is what discharges the obligation its `patches!`
/// carries, so a file whose patches never get applied is a compile error rather
/// than a set of hooks that quietly never install.
///
/// ```ignore
/// patch_groups! {
///     static PATCH_GROUPS = [timing, math, drawmode::hooks];
/// }
/// ```
#[macro_export]
macro_rules! patch_groups {
    (
        $(#[$attr:meta])*
        // Spelled out rather than `$module:path`, which can't be followed by `::`.
        $vis:vis static $name:ident = [$($($module:ident)::+),* $(,)?];
    ) => {
        $(impl $crate::patch::Grouped for $($module)::+::PatchGroup {})*

        $(#[$attr])*
        $vis static $name: &[&[&'static dyn $crate::patch::Patch]] =
            &[$($($module)::+::PATCHES),*];
    };
}
pub use crate::patch_groups;

/// Discharges the obligation `#[hook]` emits. Listing a hook twice is a
/// conflicting-implementation error, which is also a bug worth catching.
#[doc(hidden)]
#[macro_export]
macro_rules! registration {
    (hook $name:ident) => {
        impl $crate::patch::Registered for $name::Registration {}
    };
    (patch $name:ident) => {};
}
pub use crate::registration;

#[doc(hidden)]
#[macro_export]
macro_rules! patch_ref {
    (hook $name:ident) => {
        &$name::HOOK
    };
    (patch $name:ident) => {
        &$name
    };
}
pub use crate::patch_ref;
