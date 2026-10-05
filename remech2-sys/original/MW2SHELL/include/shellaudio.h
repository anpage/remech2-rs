#ifndef SHELLAUDIO_H
#define SHELLAUDIO_H

#include "types.h"

// The shell's sound, implemented on the Rust side (src/shell/audio). AudioSubsystem, MidiSequence
// and AudioSample wrap these; the original built them on Miles.
extern "C"
{
	struct ShellAudioSubsystem;
	struct ShellMidiSequence;
	struct ShellAudioSample;

	ShellAudioSubsystem* ShellAudioSubsystem_New(void);
	void ShellAudioSubsystem_Delete(ShellAudioSubsystem* p_subsystem);
	void ShellAudioSubsystem_CloseDigitalDriver(ShellAudioSubsystem* p_subsystem);
	void ShellAudioSubsystem_ApplyMidiVolume(ShellAudioSubsystem* p_subsystem);

	// Both copy p_data, and return NULL when it can't be played
	ShellMidiSequence* ShellMidiSequence_New(ShellAudioSubsystem* p_subsystem, const void* p_data, MechU32 p_size);
	ShellAudioSample* ShellAudioSample_New(ShellAudioSubsystem* p_subsystem, const void* p_data, MechU32 p_size);

	void ShellMidiSequence_Delete(ShellMidiSequence* p_sequence);
	void ShellMidiSequence_Start(ShellMidiSequence* p_sequence);
	void ShellMidiSequence_Stop(ShellMidiSequence* p_sequence);
	void ShellMidiSequence_SetVolume(ShellMidiSequence* p_sequence, MechS32 p_volume);

	void ShellAudioSample_Delete(ShellAudioSample* p_sample);
	void ShellAudioSample_SetFade(
		ShellAudioSample* p_sample,
		MechS32 p_initFadeRate,
		MechS32 p_maxFade,
		MechS32 p_startVolume,
		MechS32 p_endVolume
	);
	void ShellAudioSample_DoFade(ShellAudioSample* p_sample);
	void ShellAudioSample_EnableLoop(ShellAudioSample* p_sample);
	void ShellAudioSample_Start(ShellAudioSample* p_sample);
	void ShellAudioSample_Stop(ShellAudioSample* p_sample);
	MechS32 ShellAudioSample_IsPlaying(ShellAudioSample* p_sample);
	void ShellAudioSample_SetVolume(ShellAudioSample* p_sample, MechS32 p_volume);
}

#endif // SHELLAUDIO_H
