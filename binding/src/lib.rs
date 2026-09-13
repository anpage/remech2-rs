//! Declarative bindings to the game's DLLs: globals, hooks, thunks and data
//! patches, each declared in one place against a runtime-resolved module base.
//!
//! Disclaimer: This crate is entirely glue code and boilerplate.
//! It is mostly AI-generated with human review.

pub mod game_fn;
pub mod global;
pub mod hook;
pub mod macros;
pub mod module;
pub mod patch;
pub mod thunk;
