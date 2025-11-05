use rodio::{Sample, Source, conversions::SampleTypeConverter};

pub struct PcmSource {
    data: SampleTypeConverter<std::vec::IntoIter<u8>, Sample>,
}

impl PcmSource {
    pub fn new(initial_data: &[u8]) -> Self {
        let data = initial_data.to_vec();
        let data = SampleTypeConverter::<_, Sample>::new(data.into_iter());
        Self { data }
    }
}

impl Iterator for PcmSource {
    type Item = Sample;

    fn next(&mut self) -> Option<Self::Item> {
        self.data.next()
    }
}

impl Source for PcmSource {
    fn current_span_len(&self) -> Option<usize> {
        None
    }

    fn channels(&self) -> u16 {
        1
    }

    fn sample_rate(&self) -> u32 {
        11025
    }

    fn total_duration(&self) -> Option<std::time::Duration> {
        None
    }
}
