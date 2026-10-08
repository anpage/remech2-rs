#include "audiosample.h"
#include "audiosubsystem.h"
#include "midisequence.h"
#include "shellaudio.h"
#include "shellglobals.h"
#include "windowstate.h"

// The shell's sound classes. The original implemented them on Miles; these forward to the Rust
// side (shellaudio.h), keeping only what the shell's own state decides: whether MIDI and digital
// audio are on, and the heap blocks the objects own.

// FUNCTION: MW2SHELL 0x1003ceb0
AudioSubsystem::AudioSubsystem()
{
	m_impl = ShellAudioSubsystem_New();
}

// FUNCTION: MW2SHELL 0x1003cf5d
AudioSubsystem::~AudioSubsystem()
{
	ShellAudioSubsystem_Delete(m_impl);
}

// FUNCTION: MW2SHELL 0x1003d0fc
void AudioSubsystem::ApplyMidiVolume()
{
	ShellAudioSubsystem_ApplyMidiVolume(m_impl);
}

// FUNCTION: MW2SHELL 0x1003d198
MidiSequence::MidiSequence(AudioSubsystem* p_subsystem, void* p_data, undefined4 p_size)
{
	m_data = p_data;
	m_impl = NULL;

	if (g_midiAudio) {
		m_impl = ShellMidiSequence_New(p_subsystem->m_impl, p_data, p_size);
	}
}

// FUNCTION: MW2SHELL 0x1003d29e
MidiSequence::~MidiSequence()
{
	if (m_impl) {
		ShellMidiSequence_Delete(m_impl);
		MechHeapFree(g_primaryHeap, m_data);
	}
}

// FUNCTION: MW2SHELL 0x1003d2f7
void MidiSequence::Start()
{
	if (m_impl) {
		ShellMidiSequence_Start(m_impl);
	}
}

// FUNCTION: MW2SHELL 0x1003d341
void MidiSequence::Stop()
{
	if (m_impl) {
		ShellMidiSequence_Stop(m_impl);
	}
}

// FUNCTION: MW2SHELL 0x1003d371
void MidiSequence::SetVolume(MechS32 p_volume)
{
	if (m_impl) {
		ShellMidiSequence_SetVolume(m_impl, p_volume);
	}
}

// A sequence always loops, so it plays for as long as it exists.
// FUNCTION: MW2SHELL 0x1003d3d4
undefined MidiSequence::IsAnySequencePlaying()
{
	return m_impl ? TRUE : FALSE;
}

// FUNCTION: MW2SHELL 0x1003d419
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

// FUNCTION: MW2SHELL 0x1003d50f
AudioSample::~AudioSample()
{
	if (m_impl) {
		ShellAudioSample_Delete(m_impl);
		MechHeapFree(g_primaryHeap, m_data);
	}
}

// FUNCTION: MW2SHELL 0x1003d561
void AudioSample::SetFade(MechS32 p_initFadeRate, MechS32 p_maxFade, MechS32 p_startVolume, MechS32 p_endVolume)
{
	if (m_impl) {
		ShellAudioSample_SetFade(m_impl, p_initFadeRate, p_maxFade, p_startVolume, p_endVolume);
	}
}

// FUNCTION: MW2SHELL 0x1003d5ca
void AudioSample::DoFade()
{
	if (m_impl) {
		ShellAudioSample_DoFade(m_impl);
	}
}

// FUNCTION: MW2SHELL 0x1003d67f
void AudioSample::EnableLoop()
{
	if (m_impl) {
		ShellAudioSample_EnableLoop(m_impl);
	}
}

// FUNCTION: MW2SHELL 0x1003d6bb
void AudioSample::Start()
{
	if (m_impl) {
		ShellAudioSample_Start(m_impl);
	}
}

// FUNCTION: MW2SHELL 0x1003d709
void AudioSample::PlayAndWait()
{
	if (m_impl) {
		Start();
		while (IsPlaying() == TRUE) {
		}
	}
}

// FUNCTION: MW2SHELL 0x1003d74e
void AudioSample::Stop()
{
	if (m_impl) {
		ShellAudioSample_Stop(m_impl);
	}
}

// FUNCTION: MW2SHELL 0x1003d77e
undefined AudioSample::IsPlaying()
{
	if (m_impl && ShellAudioSample_IsPlaying(m_impl)) {
		return TRUE;
	}

	return FALSE;
}

// FUNCTION: MW2SHELL 0x1003d7c0
void AudioSample::SetVolume(MechS32 p_volume)
{
	if (m_impl) {
		ShellAudioSample_SetVolume(m_impl, p_volume);
	}
}
