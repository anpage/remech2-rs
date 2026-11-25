#include "audiosample.h"
#include "audiosubsystem.h"
#include "midisequence.h"
#include "shellaudio.h"
#include "shellglobals.h"
#include "windowstate.h"

// The shell's sound classes. The original implemented them on Miles; these forward to the Rust
// side (shellaudio.h), keeping only what the shell's own state decides: whether MIDI and digital
// audio are on, and the heap blocks the objects own.

AudioSubsystem::AudioSubsystem()
{
	m_impl = ShellAudioSubsystem_New();
}

AudioSubsystem::~AudioSubsystem()
{
	ShellAudioSubsystem_Delete(m_impl);
}

void AudioSubsystem::ApplyMidiVolume()
{
	ShellAudioSubsystem_ApplyMidiVolume(m_impl);
}

MidiSequence::MidiSequence(AudioSubsystem* p_subsystem, void* p_data, undefined4 p_size)
{
	m_data = p_data;
	m_impl = NULL;

	if (g_midiAudio) {
		m_impl = ShellMidiSequence_New(p_subsystem->m_impl, p_data, p_size);
	}
}

MidiSequence::~MidiSequence()
{
	if (m_impl) {
		ShellMidiSequence_Delete(m_impl);
		MechHeapFree(g_primaryHeap, m_data);
	}
}

void MidiSequence::Start()
{
	if (m_impl) {
		ShellMidiSequence_Start(m_impl);
	}
}

void MidiSequence::Stop()
{
	if (m_impl) {
		ShellMidiSequence_Stop(m_impl);
	}
}

void MidiSequence::SetVolume(int32_t p_volume)
{
	if (m_impl) {
		ShellMidiSequence_SetVolume(m_impl, p_volume);
	}
}

// A sequence always loops, so it plays for as long as it exists.
undefined MidiSequence::IsAnySequencePlaying()
{
	return m_impl ? TRUE : FALSE;
}

AudioSample::AudioSample(AudioSubsystem* p_subsystem, void* p_data, undefined4 p_size)
{
	m_data = p_data;
	m_impl = NULL;

	if (p_subsystem == NULL || p_data == NULL || p_size == 0) {
		return;
	}

	if (g_digitalAudio) {
		m_impl = ShellAudioSample_New(p_subsystem->m_impl, p_data, p_size);
	}
}

AudioSample::~AudioSample()
{
	if (m_impl) {
		ShellAudioSample_Delete(m_impl);
		MechHeapFree(g_primaryHeap, m_data);
	}
}

void AudioSample::SetFade(int32_t p_initFadeRate, int32_t p_maxFade, int32_t p_startVolume, int32_t p_endVolume)
{
	if (m_impl) {
		ShellAudioSample_SetFade(m_impl, p_initFadeRate, p_maxFade, p_startVolume, p_endVolume);
	}
}

void AudioSample::DoFade()
{
	if (m_impl) {
		ShellAudioSample_DoFade(m_impl);
	}
}

void AudioSample::EnableLoop()
{
	if (m_impl) {
		ShellAudioSample_EnableLoop(m_impl);
	}
}

void AudioSample::Start()
{
	if (m_impl) {
		ShellAudioSample_Start(m_impl);
	}
}

void AudioSample::PlayAndWait()
{
	if (m_impl) {
		Start();
		while (IsPlaying() == TRUE) {
		}
	}
}

void AudioSample::Stop()
{
	if (m_impl) {
		ShellAudioSample_Stop(m_impl);
	}
}

undefined AudioSample::IsPlaying()
{
	if (m_impl && ShellAudioSample_IsPlaying(m_impl)) {
		return TRUE;
	}

	return FALSE;
}

void AudioSample::SetVolume(int32_t p_volume)
{
	if (m_impl) {
		ShellAudioSample_SetVolume(m_impl, p_volume);
	}
}
