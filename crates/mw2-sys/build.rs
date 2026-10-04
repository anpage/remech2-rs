use std::{
    env, fs,
    path::{Path, PathBuf},
};

const SYSTEM_TYPES: &str = "HWND|HWND__|HINSTANCE|HINSTANCE__|HANDLE|BITMAPINFOHEADER|tagBITMAPINFOHEADER|\
                           RGBQUAD|tagRGBQUAD|FILE|_iobuf|BOOL|BYTE|WORD|DWORD|LONG|LONG_PTR|UINT|UINT_PTR|\
                           WPARAM|LPARAM|HWAVEOUT|HWAVEOUT__|LPHWAVEOUT";
const SHARED_FILES: &str = ".*/src/original/(util|common|mss|smacker)/.*";

fn main() {
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../src/original");
    println!("cargo::rerun-if-changed={}", root.display());

    let common = [
        root.join("util"),
        root.join("common/include"),
        root.join("mss"),
        root.join("smacker"),
    ];

    shared_bindings(&common);

    let mut objects = compile(
        &common,
        None,
        sources(&root.join("common/src")).chain(sources(&root.join("util"))),
    );
    objects.extend(module(
        &root,
        &common,
        "MW2",
        "mw2",
        Some("sim_names.h"),
        false,
    ));
    objects.extend(module(&root, &common, "MW2SHELL", "mw2shell", None, true));

    cc::Build::new().objects(objects).compile("original");
}

fn shared_bindings(common: &[PathBuf]) {
    let out = PathBuf::from(env::var("OUT_DIR").unwrap());
    let wrapper = out.join("shared.h");
    let headers: String = std::iter::once("#include <stdio.h>\n".to_owned()).chain(common
        .iter()
        .flat_map(|dir| fs::read_dir(dir).unwrap())
        .map(|e| e.unwrap().path())
        .filter(|p| p.extension().is_some_and(|e| e == "h"))
        .map(|p| format!("#include \"{}\"\n", p.display())))
        .collect();
    fs::write(&wrapper, headers).unwrap();

    bindgen::Builder::default()
        .header(wrapper.to_str().unwrap())
        .wrap_unsafe_ops(true)
        .clang_arg("--target=i686-pc-windows-gnu")
        .clang_args(common.iter().map(|p| format!("-I{}", p.display())))
        .allowlist_file(SHARED_FILES)
        .allowlist_type(SYSTEM_TYPES)
        .derive_default(true)
        .generate()
        .unwrap()
        .write_to_file(out.join("shared.rs"))
        .unwrap();
}

fn module(
    root: &Path,
    common: &[PathBuf],
    dir: &str,
    name: &str,
    names: Option<&str>,
    cpp: bool,
) -> Vec<PathBuf> {
    let dir = root.join(dir);
    let include = dir.join("include");
    let includes: Vec<_> = common.iter().cloned().chain([include.clone()]).collect();
    let forced = names.map(|n| dir.join(n));

    let objects = compile(&includes, forced.as_deref(), sources(&dir.join("src")));

    let out = PathBuf::from(env::var("OUT_DIR").unwrap());
    let wrapper = out.join(format!("{name}.h"));
    let headers: String = fs::read_dir(&include)
        .unwrap()
        .map(|e| e.unwrap().path())
        .filter(|p| p.extension().is_some_and(|e| e == "h"))
        .map(|p| format!("#include \"{}\"\n", p.display()))
        .collect();
    fs::write(&wrapper, headers).unwrap();

    let mut bindings = bindgen::Builder::default()
        .header(wrapper.to_str().unwrap())
        .wrap_unsafe_ops(true)
        .clang_arg("--target=i686-pc-windows-gnu")
        .clang_args(includes.iter().map(|p| format!("-I{}", p.display())))
        .allowlist_file(format!(".*/src/original/{}/.*", dir.file_name().unwrap().to_str().unwrap()))
        .blocklist_file(SHARED_FILES)
        .blocklist_type(SYSTEM_TYPES)
        .raw_line("use super::shared::*;")
        .derive_default(true);
    if let Some(forced) = &forced {
        bindings = bindings.clang_args(["-include", forced.to_str().unwrap()]);
    }
    if cpp {
        bindings = bindings.clang_args(["-x", "c++"]);
    }
    bindings
        .generate()
        .unwrap()
        .write_to_file(out.join(format!("{name}.rs")))
        .unwrap();

    objects
}

fn compile(
    includes: &[PathBuf],
    forced: Option<&Path>,
    files: impl IntoIterator<Item = PathBuf>,
) -> Vec<PathBuf> {
    let (cpp, c): (Vec<_>, Vec<_>) = files
        .into_iter()
        .partition(|p| p.extension().is_some_and(|e| e == "cpp"));

    let mut objects = Vec::new();
    for (files, cpp) in [(c, false), (cpp, true)] {
        if files.is_empty() {
            continue;
        }
        let mut build = base(includes, cpp);
        if let Some(forced) = forced {
            build.flag("-include").flag(forced);
        }
        objects.extend(build.files(files).compile_intermediates());
    }
    objects
}

fn base(includes: &[PathBuf], cpp: bool) -> cc::Build {
    let mut build = cc::Build::new();
    build
        .cpp(cpp)
        .includes(includes)
        .opt_level(1)
        .warnings(false)
        .flag("-fwrapv")
        .flag("-fno-strict-aliasing")
        .flag("-fms-extensions");
    if cpp {
        build
            .flag("-fno-exceptions")
            .flag("-fno-rtti")
            .flag("-fcheck-new")
            .flag("-fpermissive");
    } else {
        build
            .flag("-Wno-error=implicit-function-declaration")
            .flag("-Wno-error=int-conversion")
            .flag("-Wno-error=incompatible-pointer-types");
    }
    build
}

fn sources(dir: &Path) -> impl Iterator<Item = PathBuf> {
    fs::read_dir(dir)
        .unwrap()
        .map(|e| e.unwrap().path())
        .filter(|p| p.extension().is_some_and(|e| e == "c" || e == "cpp"))
}
