//! Attribute macros for `remech2`'s binding layer.
//!
//! These exist as attribute macros rather than `macro_rules!` so that rustfmt
//! formats hook bodies: it treats the contents of a brace-delimited macro
//! invocation as opaque tokens, but an attributed `fn` is an ordinary item.
//!
//! The expansions name `::binding::*`, so the `binding` crate must be a
//! dependency of the crate using these, under that name.
//!
//! Disclaimer: This crate is entirely glue code and boilerplate.
//! It is mostly AI-generated with human review.

use proc_macro::TokenStream;
use proc_macro2::{Span, TokenStream as TokenStream2};
use quote::{format_ident, quote_spanned};
use syn::{
    Error, FnArg, Ident, ItemFn, LitInt, LitStr, Token,
    parse::{Parse, ParseStream},
    parse_macro_input,
    spanned::Spanned,
};

/// Declares a detour over a game function.
///
/// The body can call `original(..)` to reach the function that was replaced.
/// The address is an RVA into the file's `MODULE`, or into a named module with
/// `#[hook(rva = 0x1234, module = OTHER_MODULE)]`.
///
/// ```ignore
/// #[hook(rva = 0x0006ae5a)]
/// unsafe extern "cdecl" fn guide_missile_to_target(shot: *mut Shot) {
///     unsafe { original(shot) };
/// }
/// ```
#[proc_macro_attribute]
pub fn hook(attr: TokenStream, item: TokenStream) -> TokenStream {
    expand(attr, item, Kind::Function)
}

/// Declares a `RawDetour` with a body, for an address that isn't a function entry.
///
/// As with [`macro@hook`], the body can call `original(..)`.
#[proc_macro_attribute]
pub fn raw_hook(attr: TokenStream, item: TokenStream) -> TokenStream {
    expand(attr, item, Kind::Raw)
}

enum Kind {
    Function,
    Raw,
}

/// `rva = 0x1234`, plus an optional `module = OTHER_MODULE` override.
struct Args {
    rva: LitInt,
    module: Ident,
}

impl Parse for Args {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let mut rva = None;
        let mut module = None;

        while !input.is_empty() {
            let key: Ident = input.parse()?;
            input.parse::<Token![=]>()?;
            match key.to_string().as_str() {
                "rva" => rva = Some(input.parse()?),
                "module" => module = Some(input.parse()?),
                other => {
                    let message = format!("expected `rva` or `module`, found `{other}`");
                    return Err(Error::new(key.span(), message));
                }
            }
            if !input.is_empty() {
                input.parse::<Token![,]>()?;
            }
        }

        Ok(Args {
            rva: rva.ok_or_else(|| Error::new(Span::call_site(), "missing `rva = 0x1234`"))?,
            // The file's own `MODULE`, resolved at the call site.
            module: module.unwrap_or_else(|| Ident::new("MODULE", Span::call_site())),
        })
    }
}

fn expand(attr: TokenStream, item: TokenStream, kind: Kind) -> TokenStream {
    let args = parse_macro_input!(attr as Args);
    let func = parse_macro_input!(item as ItemFn);

    match expand_hook(args, func, kind) {
        Ok(tokens) => tokens.into(),
        Err(e) => e.to_compile_error().into(),
    }
}

fn expand_hook(args: Args, func: ItemFn, kind: Kind) -> syn::Result<TokenStream2> {
    let ItemFn {
        attrs,
        vis,
        sig,
        block,
    } = func;

    if sig.unsafety.is_none() {
        return Err(Error::new(sig.span(), "a hook must be `unsafe`"));
    }
    if sig.abi.is_none() {
        return Err(Error::new(
            sig.span(),
            "a hook must name an ABI, such as `extern \"stdcall\"`",
        ));
    }
    if !sig.generics.params.is_empty() || sig.generics.where_clause.is_some() {
        return Err(Error::new(sig.generics.span(), "a hook can't be generic"));
    }
    if let Some(variadic) = &sig.variadic {
        return Err(Error::new(
            variadic.span(),
            "a variadic can't implement `retour::Function`; use `raw_detour!` instead",
        ));
    }

    // The signature the game's own code has at this address.
    let unsafety = &sig.unsafety;
    let abi = &sig.abi;
    let output = &sig.output;
    let arg_types = sig
        .inputs
        .iter()
        .map(|arg| match arg {
            FnArg::Typed(arg) => Ok(&*arg.ty),
            FnArg::Receiver(arg) => Err(Error::new(arg.span(), "a hook can't take `self`")),
        })
        .collect::<syn::Result<Vec<_>>>()?;

    // Named independently of the detour's own parameters, which may be wildcards.
    let arg_names = (0..arg_types.len())
        .map(|i| format_ident!("arg{i}"))
        .collect::<Vec<_>>();

    let name = &sig.ident;
    let name_str = LitStr::new(&name.to_string(), name.span());
    let Args { rva, module } = args;

    // Taken from the caller's own source: tokens stamped with a synthetic span
    // count as an external macro expansion, and rustc's dead-code pass won't
    // report on those — so a hook missing from its `patches!` would go unnoticed.
    let span = name.span();

    let declaration = match kind {
        Kind::Function => quote_spanned! {span=>
            pub static HOOK: ::binding::hook::Hook<Sig> =
                ::binding::hook::Hook::new(&#module, RVA, #name_str, super::#name);
        },
        Kind::Raw => quote_spanned! {span=>
            pub static HOOK: ::binding::hook::RawHook = ::binding::hook::RawHook::new(
                &#module,
                RVA,
                #name_str,
                super::#name as *const (),
            );
        },
    };

    let original = match kind {
        Kind::Function => quote_spanned! {span=> #name::HOOK.original() },
        Kind::Raw => quote_spanned! {span=> #name::HOOK.original::<#name::Sig>() },
    };

    Ok(quote_spanned! {span=>
        #[doc(hidden)]
        #vis mod #name {
            use super::*;

            /// The signature of the function we replace.
            pub type Sig = #unsafety #abi fn(#(#arg_types),*) #output;

            /// Offset into the module, as shown in Ghidra.
            pub const RVA: usize = #rva;

            /// Marker `patches!` implements `Registered` for.
            pub struct Registration;

            #declaration
        }

        // A hook that reaches no `patches!` list is never installed. The
        // dead-code lint can't catch that on its own: calling the detour from
        // anywhere makes `HOOK` live whether or not it's registered.
        const _: () = {
            const fn assert_registered<T: ::binding::patch::Registered>() {}
            assert_registered::<#name::Registration>();
        };

        #(#attrs)*
        #vis #sig {
            /// Calls the function this hook replaced.
            unsafe fn original(#(#arg_names: #arg_types),*) #output {
                unsafe { (#original)(#(#arg_names),*) }
            }
            // Bodies that reimplement rather than wrap never call `original`.
            let _ = original;
            #block
        }
    })
}
