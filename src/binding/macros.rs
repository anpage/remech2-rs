/// Declares game globals against the file's `MODULE`.
macro_rules! globals {
    ($(
        $(#[$attr:meta])*
        $vis:vis static $name:ident: $ty:ty = $rva:literal;
    )*) => {$(
        $(#[$attr])*
        $vis static $name: $crate::binding::global::Global<$ty> =
            $crate::binding::global::Global::new(&MODULE, $rva);
    )*};
}
pub(crate) use globals;

macro_rules! game_fns {
    ($(
        $(#[$attr:meta])*
        $vis:vis static $name:ident: $sig:ty = $rva:literal;
    )*) => {$(
        $(#[$attr])*
        $vis static $name: $crate::binding::GameFn<$sig> =
            $crate::binding::GameFn::new(&MODULE, $rva);
    )*};
}
pub(crate) use game_fns;

/// Declares a detour over a game function.
///
/// The body can call `original(..)` to reach the function that was replaced. The
/// address is an RVA into the file's `MODULE`, or into a named module with
/// `#[rva(0x1234 in OTHER_MODULE)]`.
macro_rules! hook {
    // Explicit module.
    (
        $(#[doc = $doc:literal])*
        #[rva($rva:literal in $module:ident)]
        $(#[$attr:meta])*
        $vis:vis unsafe extern $abi:literal fn $name:ident(
            $($arg:ident: $arg_ty:ty),* $(,)?
        ) $(-> $ret:ty)? $body:block
    ) => {
        #[doc(hidden)]
        $vis mod $name {
            use super::*;

            /// The signature of the function we replace.
            pub type Sig = unsafe extern $abi fn($($arg_ty),*) $(-> $ret)?;

            /// Offset into the module, as shown in Ghidra.
            pub const RVA: usize = $rva;

            pub static HOOK: $crate::binding::Hook<Sig> =
                $crate::binding::Hook::new(&$module, RVA, stringify!($name), super::$name);
        }

        $(#[doc = $doc])*
        $(#[$attr])*
        $vis unsafe extern $abi fn $name($($arg: $arg_ty),*) $(-> $ret)? {
            /// Calls the function this hook replaced.
            #[allow(dead_code)]
            unsafe fn original($($arg: $arg_ty),*) $(-> $ret)? {
                unsafe { ($name::HOOK.original())($($arg),*) }
            }
            $body
        }
    };

    // The file's `MODULE`.
    (
        $(#[doc = $doc:literal])*
        #[rva($rva:literal)]
        $(#[$attr:meta])*
        $vis:vis unsafe extern $abi:literal fn $name:ident(
            $($arg:ident: $arg_ty:ty),* $(,)?
        ) $(-> $ret:ty)? $body:block
    ) => {
        $crate::binding::hook! {
            $(#[doc = $doc])*
            #[rva($rva in MODULE)]
            $(#[$attr])*
            $vis unsafe extern $abi fn $name($($arg: $arg_ty),*) $(-> $ret)? $body
        }
    };
}
pub(crate) use hook;

/// Declares a `RawDetour` with a body, for an address that isn't a function entry.
///
/// As with [`hook!`], the body can call `original(..)`.
macro_rules! raw_hook {
    (
        $(#[doc = $doc:literal])*
        #[rva($rva:literal $(in $module:ident)?)]
        $(#[$attr:meta])*
        $vis:vis unsafe extern $abi:literal fn $name:ident(
            $($arg:ident: $arg_ty:ty),* $(,)?
        ) $(-> $ret:ty)? $body:block
    ) => {
        #[doc(hidden)]
        $vis mod $name {
            use super::*;

            pub type Sig = unsafe extern $abi fn($($arg_ty),*) $(-> $ret)?;
            pub const RVA: usize = $rva;

            pub static HOOK: $crate::binding::RawHook = $crate::binding::RawHook::new(
                &$crate::binding::pick_module!($($module)?),
                RVA,
                stringify!($name),
                super::$name as *const (),
            );
        }

        $(#[doc = $doc])*
        $(#[$attr])*
        $vis unsafe extern $abi fn $name($($arg: $arg_ty),*) $(-> $ret)? {
            /// Calls the code this hook displaced.
            #[allow(dead_code)]
            unsafe fn original($($arg: $arg_ty),*) $(-> $ret)? {
                unsafe { ($name::HOOK.original::<$name::Sig>())($($arg),*) }
            }
            $body
        }
    };
}
pub(crate) use raw_hook;

/// Points an address at a detour that's defined elsewhere, with no way back to
/// the original. For detours that can't implement `Function`, such as variadics.
macro_rules! raw_detour {
    ($(
        $(#[$attr:meta])*
        $vis:vis static $name:ident at $rva:literal $(in $module:ident)? => $detour:path;
    )*) => {$(
        $(#[$attr])*
        $vis static $name: $crate::binding::RawHook = $crate::binding::RawHook::new(
            &$crate::binding::pick_module!($($module)?),
            $rva,
            stringify!($name),
            $detour as *const (),
        );
    )*};
}
pub(crate) use raw_detour;

/// `MODULE` unless an override was given. Keeps the `in OTHER` suffix from doubling every rule.
#[doc(hidden)]
macro_rules! pick_module {
    () => {
        MODULE
    };
    ($module:ident) => {
        $module
    };
}
pub(crate) use pick_module;

/// Declares a table of import thunks to overwrite.
macro_rules! thunks {
    (
        $(#[$attr:meta])*
        $vis:vis static $name:ident $(in $module:ident)? = [
            $($rva:literal => $replacement:path),* $(,)?
        ];
    ) => {
        $(#[$attr])*
        $vis static $name: $crate::binding::Table<$crate::binding::Thunk> =
            $crate::binding::Table::new(
                stringify!($name),
                &[$($crate::binding::Thunk::new(
                    &$crate::binding::pick_module!($($module)?),
                    $rva,
                    stringify!($replacement),
                    $replacement as *const (),
                )),*],
            );
    };
}
pub(crate) use thunks;

/// Declares a table of raw byte writes into the game's data.
macro_rules! data_patches {
    (
        $(#[$attr:meta])*
        $vis:vis static $name:ident $(in $module:ident)? = [
            $($label:ident: $rva:literal => $bytes:expr),* $(,)?
        ];
    ) => {
        $(#[$attr])*
        $vis static $name: $crate::binding::Table<$crate::binding::DataPatch> =
            $crate::binding::Table::new(
                stringify!($name),
                &[$($crate::binding::DataPatch::new(
                    &$crate::binding::pick_module!($($module)?),
                    $rva,
                    stringify!($label),
                    $bytes,
                )),*],
            );
    };
}
pub(crate) use data_patches;

/// Collects a file's bindings into the list its parent module registers.
///
/// `hook <name>` takes anything declared with [`hook!`] or [`raw_hook!`];
/// `patch <name>` takes a named static, such as a thunk table, a data patch
/// table or a [`raw_detour!`].
macro_rules! patches {
    (
        $(#[$attr:meta])*
        $vis:vis static $name:ident = [$($kind:ident $item:ident),* $(,)?];
    ) => {
        $(#[$attr])*
        $vis static $name: &[&'static dyn $crate::binding::Patch] =
            &[$($crate::binding::patch_ref!($kind $item)),*];
    };
}
pub(crate) use patches;

#[doc(hidden)]
macro_rules! patch_ref {
    (hook $name:ident) => {
        &$name::HOOK
    };
    (patch $name:ident) => {
        &$name
    };
}
pub(crate) use patch_ref;
