use std::sync::Mutex;

use anyhow::Result;
use rodio::{
    DeviceSinkBuilder, MixerDeviceSink, SampleRate, Source,
    mixer::Mixer,
    nz,
    source::{LimitSettings, Zero},
};

struct Output {
    stream: MixerDeviceSink,
    primary: Mixer,
}

static OUTPUT: Mutex<Option<Output>> = Mutex::new(None);

pub fn primary() -> Result<Mixer> {
    let mut output = OUTPUT.lock().unwrap();

    if let Some(output) = &*output {
        return Ok(output.primary.clone());
    }

    let stream = DeviceSinkBuilder::from_default_device()?
        .with_channels(nz!(2))
        .open_stream()?;
    let rate = stream.config().sample_rate();
    let (primary, source) = rodio::mixer::mixer(nz!(2), rate);

    stream.mixer().add(
        source
            .mix(Zero::new(nz!(2), rate))
            .limit(LimitSettings::gaming()),
    );

    *output = Some(Output {
        stream,
        primary: primary.clone(),
    });

    Ok(primary)
}

pub fn sample_rate() -> Option<SampleRate> {
    OUTPUT
        .lock()
        .unwrap()
        .as_ref()
        .map(|o| o.stream.config().sample_rate())
}
