mod driver;
pub mod interface;
mod sample;
mod storage;
mod voice;

pub fn shutdown() {
    storage::shutdown();
}
