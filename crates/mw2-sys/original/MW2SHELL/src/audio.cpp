#include "audiosample.h"
#include "audiosubsystem.h"
#include "midisequence.h"
#include "mss.h"
#include "shellglobals.h"
#include "soundconfig.h"
#include "videosound.h"
#include "windowstate.h"

#include <windows.h>

// Miles sample formats (DIG_F_*).
enum {
	c_formatMono8 = 0,
	c_formatMono16 = 1,
	c_formatStereo8 = 2,
	c_formatStereo16 = 3
};

DECOMP_SIZE_ASSERT(AudioSubsystem, 0x15)
DECOMP_SIZE_ASSERT(AudioSample, 0x2c)

// FUNCTION: MW2SHELL 0x1003ceb0
AudioSubsystem::AudioSubsystem()
{
	m_currentMidiSequence = NULL;
	m_midiDriver = NULL;
	m_digitalDriver = NULL;

	if (g_midiAudio || g_digitalAudio) {
		m_milesStarted = 1;
	}
	else {
		m_milesStarted = 0;
	}

	if (g_midiAudio) {
		if (AIL_midiOutOpen(&m_midiDriver, NULL, -1) && AIL_midiOutOpen(&m_midiDriver, NULL, 0)) {
			m_midiDriver = NULL;
		}
	}
}

// FUNCTION: MW2SHELL 0x1003cf5d
AudioSubsystem::~AudioSubsystem()
{
	if (m_milesStarted == 1) {
		AIL_shutdown();
	}
}

// FUNCTION: MW2SHELL 0x1003cf89
HDIGDRIVER AudioSubsystem::GetDigitalDriver()
{
	AILPCMWAVEFORMAT format;

	if (m_digitalDriver) {
		return m_digitalDriver;
	}

	if (!m_milesStarted || !g_digitalAudio) {
		return NULL;
	}

	format.wf.wFormatTag = AIL_WAVE_FORMAT_PCM;
	format.wf.nChannels = 1;
	format.wf.nSamplesPerSec = 11025;
	format.wf.nAvgBytesPerSec = 11025;
	format.wf.nBlockAlign = 1;
	format.wBitsPerSample = 8;

	if (AIL_waveOutOpen(&m_digitalDriver, &m_waveOut, 0, (AILWAVEFORMAT*) &format)) {
		m_digitalDriver = NULL;
	}

	return m_digitalDriver;
}

// FUNCTION: MW2SHELL 0x1003d032
void AudioSubsystem::CloseDigitalDriver()
{
	if (m_digitalDriver) {
		AIL_waveOutClose(m_digitalDriver);
	}

	m_digitalDriver = NULL;
}

// FUNCTION: MW2SHELL 0x1003d06c
undefined AudioSubsystem::IsAnySamplePlaying()
{
	if (!m_digitalDriver) {
		return FALSE;
	}

	if (!AIL_active_sample_count(m_digitalDriver)) {
		return FALSE;
	}

	return TRUE;
}

// FUNCTION: MW2SHELL 0x1003d0b4
undefined AudioSubsystem::IsAnySequencePlaying()
{
	if (!m_midiDriver) {
		return FALSE;
	}

	if (!AIL_active_sequence_count(m_midiDriver)) {
		return FALSE;
	}

	return TRUE;
}

// FUNCTION: MW2SHELL 0x1003d0fc
void AudioSubsystem::ApplyMidiVolume()
{
	if (m_currentMidiSequence) {
		m_currentMidiSequence->ApplyCurrentVolume();
	}
}

// FUNCTION: MW2SHELL 0x1003d12a
MechS32 AudioSubsystem::GetMidiVolume()
{
	return g_soundConfig.m_midiVolume;
}

// FUNCTION: MW2SHELL 0x1003d145
MechS32 AudioSubsystem::GetEffectsVolume()
{
	return g_soundConfig.m_effectsVolume;
}

// FUNCTION: MW2SHELL 0x1003d160
void AudioSubsystem::StopMidiSequence()
{
	if (m_currentMidiSequence) {
		m_currentMidiSequence->Stop();
		m_currentMidiSequence = NULL;
	}
}

// FUNCTION: MW2SHELL 0x1003d198
MidiSequence::MidiSequence(AudioSubsystem* p_subsystem, void* p_data, undefined4 p_size)
{
	MechS32 sequenceNum;

	m_subsystem = p_subsystem;
	m_data = p_data;
	m_size = p_size;
	m_sequence = NULL;

	if (m_subsystem->m_midiDriver == NULL) {
		return;
	}

	m_sequence = AIL_allocate_sequence_handle(m_subsystem->m_midiDriver);
	if (m_sequence == NULL) {
		return;
	}

	sequenceNum = 0;
	if (AIL_init_sequence(m_sequence, m_data, sequenceNum) <= 0) {
		m_sequence = NULL;
		return;
	}

	SetLoopCount(0);
	SetVolume(50);
}

// FUNCTION: MW2SHELL 0x1003d25c
void MidiSequence::SetLoopCount(MechS32 p_loopCount)
{
	m_loopCount = p_loopCount;
	if (m_sequence) {
		AIL_set_sequence_loop_count(m_sequence, m_loopCount);
	}
}

// FUNCTION: MW2SHELL 0x1003d29e
MidiSequence::~MidiSequence()
{
	if (m_sequence) {
		Stop();
		m_subsystem->m_currentMidiSequence = NULL;
		AIL_release_sequence_handle(m_sequence);
		MechHeapFree(g_primaryHeap, m_data);
	}
}

// FUNCTION: MW2SHELL 0x1003d2f7
void MidiSequence::Start()
{
	if (m_sequence) {
		AIL_start_sequence(m_sequence);
		SetLoopCount(m_loopCount);
		m_subsystem->m_currentMidiSequence = this;
	}
}

// FUNCTION: MW2SHELL 0x1003d341
void MidiSequence::Stop()
{
	if (m_sequence) {
		AIL_stop_sequence(m_sequence);
	}
}

// FUNCTION: MW2SHELL 0x1003d371
void MidiSequence::SetVolume(MechS32 p_volume)
{
	m_volume = p_volume;
	if (m_sequence) {
		AIL_set_sequence_volume(m_sequence, (MechS32) (((m_subsystem->GetMidiVolume() * m_volume) >> 16) * 1.27), 0);
	}
}

// FUNCTION: MW2SHELL 0x1003d3d4
undefined MidiSequence::IsAnySequencePlaying()
{
	return m_subsystem->IsAnySequencePlaying();
}

// FUNCTION: MW2SHELL 0x1003d3f4
void MidiSequence::ApplyCurrentVolume()
{
	SetVolume(m_volume);
}

// FUNCTION: MW2SHELL 0x1003d419
AudioSample::AudioSample(AudioSubsystem* p_subsystem, void* p_data, undefined4 p_size)
{
	m_subsystem = p_subsystem;
	m_data = p_data;
	m_size = p_size;
	m_sample = NULL;
	m_volume = 1;
	m_loopCount = 1;
	m_fadeRate = -1;
	m_initFadeRate = -1;
	m_maxFade = -1;
	m_startVolume = m_volume;
	m_endVolume = m_startVolume;

	if (p_subsystem == NULL || p_data == NULL || p_size == 0) {
		return;
	}

	m_sample = AIL_allocate_sample_handle(p_subsystem->GetDigitalDriver());
	if (m_sample == NULL) {
		return;
	}

	AIL_init_sample(m_sample);
	AIL_set_sample_file(m_sample, m_data, -1);
}

// FUNCTION: MW2SHELL 0x1003d50f
AudioSample::~AudioSample()
{
	if (m_sample) {
		AIL_end_sample(m_sample);
		AIL_release_sample_handle(m_sample);
		MechHeapFree(g_primaryHeap, m_data);
	}
}

// FUNCTION: MW2SHELL 0x1003d561
void AudioSample::SetFade(MechS32 p_initFadeRate, MechS32 p_maxFade, MechS32 p_startVolume, MechS32 p_endVolume)
{
	if (!m_sample) {
		return;
	}

	m_fadeRate = m_initFadeRate = p_initFadeRate;
	m_maxFade = p_maxFade;
	m_startVolume = p_startVolume;
	m_endVolume = p_endVolume;
	SetVolume(m_startVolume);
}

// FUNCTION: MW2SHELL 0x1003d5ca
void AudioSample::DoFade()
{
	if (!m_sample) {
		return;
	}

	m_fadeRate--;
	if (m_fadeRate == 0) {
		if (!m_maxFade) {
			return;
		}

		m_maxFade--;
		if (m_maxFade > 0) {
			m_fadeRate = m_initFadeRate;

			if (m_endVolume == m_volume) {
				return;
			}

			if (m_endVolume > m_volume) {
				m_volume++;
			}
			else {
				m_volume--;
			}

			SetVolume(m_volume);
		}
	}
}

// FUNCTION: MW2SHELL 0x1003d67f
void AudioSample::EnableLoop()
{
	if (m_sample) {
		m_loopCount = 0;
		AIL_set_sample_loop_count(m_sample, 0);
	}
}

// FUNCTION: MW2SHELL 0x1003d6bb
void AudioSample::Start()
{
	if (m_sample) {
		SetLoopCount(m_loopCount);
		SetVolume(m_volume);
		AIL_start_sample(m_sample);
	}
}

// FUNCTION: MW2SHELL 0x1003d709
void AudioSample::PlayAndWait()
{
	if (m_sample) {
		Start();
		while (IsPlaying() == TRUE) {
		}
	}
}

// FUNCTION: MW2SHELL 0x1003d74e
void AudioSample::Stop()
{
	if (m_sample) {
		AIL_stop_sample(m_sample);
	}
}

// FUNCTION: MW2SHELL 0x1003d77e
undefined AudioSample::IsPlaying()
{
	if (m_sample && AIL_sample_status(m_sample) == SMP_PLAYING) {
		return TRUE;
	}

	return FALSE;
}

// FUNCTION: MW2SHELL 0x1003d7c0
void AudioSample::SetVolume(MechS32 p_volume)
{
	if (m_sample) {
		if (p_volume > 127) {
			m_volume = 127;
		}
		else if (p_volume < 0) {
			m_volume = 0;
		}
		else {
			m_volume = p_volume;
		}

		AIL_set_sample_volume(m_sample, (m_subsystem->GetEffectsVolume() * m_volume) >> 16);
	}
}

// FUNCTION: MW2SHELL 0x1003d842
void AudioSample::SetLoopCount(MechS32 p_loopCount)
{
	m_loopCount = p_loopCount;
	if (m_sample) {
		AIL_set_sample_loop_count(m_sample, m_loopCount);
	}
}

// Allocates a Miles sample for streaming stereo or mono, 16-bit (p_wide) or 8-bit samples, with two buffers of at least
// p_size samples. Without a digital driver, a sample or the buffers, it stays silent.
// Not 100%: the stack slots of minimum and format are permuted.
// FUNCTION: MW2SHELL 0x1003d884
VideoSound::VideoSound(AudioSubsystem* p_subsystem, MechS32 p_stereo, MechS32 p_wide, MechS32 p_size)
{
	MechS32 minimum;
	MechS32 format;

	m_subsystem = p_subsystem;
	m_sample = NULL;
	m_buffer0 = NULL;
	m_buffer1 = NULL;
	m_readyBuffer = -1;

	if (!m_subsystem->m_digitalDriver) {
		return;
	}

	m_sample = AIL_allocate_sample_handle(m_subsystem->m_digitalDriver);
	if (!m_sample) {
		return;
	}

	if (p_stereo) {
		if (p_wide) {
			format = c_formatStereo16;
		}
		else {
			format = c_formatStereo8;
		}
	}
	else if (p_wide) {
		format = c_formatMono16;
	}
	else {
		format = c_formatMono8;
	}

	minimum = AIL_minimum_sample_buffer_size(m_subsystem->m_digitalDriver, m_subsystem->m_playbackRate, format);
	if (minimum <= p_size) {
		if (format == c_formatStereo16) {
			p_size <<= 2;
		}
		else if (format != c_formatMono8) {
			p_size <<= 1;
		}

		m_buffer0 = MechHeapAlloc(g_primaryHeap, p_size);
		m_buffer1 = MechHeapAlloc(g_primaryHeap, p_size);
	}

	if (!m_buffer0 || !m_buffer1) {
		if (m_buffer0) {
			MechHeapFree(g_primaryHeap, m_buffer0);
		}
		AIL_release_sample_handle(m_sample);
		if (!m_sample) {
			// Both branches are empty in the original.
		}
		else {
		}
		return;
	}

	AIL_init_sample(m_sample);
	AIL_set_sample_type(m_sample, format, p_wide != 0);
}

// FUNCTION: MW2SHELL 0x1003da54
VideoSound::~VideoSound()
{
	if (m_sample != NULL) {
		AIL_end_sample(m_sample);
		AIL_release_sample_handle(m_sample);
		if (m_buffer0 != NULL) {
			MechHeapFree(g_primaryHeap, m_buffer0);
		}
		if (m_buffer1 != NULL) {
			MechHeapFree(g_primaryHeap, m_buffer1);
		}
	}
}

// FUNCTION: MW2SHELL 0x1003dad5
undefined4 VideoSound::IsBufferReady()
{
	if (m_sample) {
		if (m_readyBuffer == -1) {
			m_readyBuffer = AIL_sample_buffer_ready(m_sample);
		}
		if (m_readyBuffer != -1) {
			return TRUE;
		}
	}

	return FALSE;
}

// FUNCTION: MW2SHELL 0x1003db31
void* VideoSound::GetReadyBuffer()
{
	if (m_sample) {
		if (m_readyBuffer == -1) {
			m_readyBuffer = AIL_sample_buffer_ready(m_sample);
		}
		if (m_readyBuffer != -1) {
			return (&m_buffer0)[m_readyBuffer];
		}
	}

	return NULL;
}

// FUNCTION: MW2SHELL 0x1003db95
void VideoSound::LoadBuffer(void* p_buffer, MechU32 p_size)
{
	// The two adjacent buffer fields are selected by Miles' ready-buffer index.
	if (m_sample) {
		if (m_readyBuffer == -1 || (&m_buffer0)[m_readyBuffer] != p_buffer) {
			return;
		}
		AIL_load_sample_buffer(m_sample, m_readyBuffer, p_buffer, p_size);
		m_readyBuffer = -1;
	}
}
