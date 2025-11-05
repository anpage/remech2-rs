mod driver;
pub mod interface;
mod pcm_source;
mod sample;
mod storage;

pub fn shutdown() {
    storage::shutdown();
}
