fn main() {
    let _build = cxx_build::bridge("src/librespot.rs");
    println!("cargo:rerun-if-changed=src/librespot.rs");
}
