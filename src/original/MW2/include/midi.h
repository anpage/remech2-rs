#ifndef MIDI_H
#define MIDI_H

#include "decomp.h"
#include "mss.h"
#include "types.h"

// The functions and globals of midi.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS8 g_lockedChannel;
	extern HMDIDRIVER g_midiDriver;
	extern MechS32 g_midiShutDown;
	extern MechS32 g_midiInitialized;
	extern MechS16 g_midiSequenceIds[8];
	extern HSEQUENCE g_midiSequences[8];

	void StopMidiSequences(void);
	void PauseMidiSequences(void);
	void ResumeMidiSequences(void);
	MechS32 InitializeMidi(void);
	MechS32 PlayMidiSequence(undefined4 p_unk0x00, MechS16 p_id, MechS16 p_sequenceNum);
	void ShutdownMidi(void);
	MechS16 AnyMidiPlaying(void);
	void SetMidiVolume(MechS16 p_volume);
	void ApplyMidiVolume(void);
	MechS32 FUN_100219f5(
		MechS32 p_delay,
		MechS32 p_bearing,
		MechS32 p_id,
		MechU32 p_volume,
		MechS32 p_pan,
		MechS32 p_flags
	);
	void StartEngineNote(void);
	void UpdateEngineNote(MechU32 p_pitch);
	void StopEngineNote(void);
	void MuteEngineNote(void);
	void UnmuteEngineNote(void);

#ifdef __cplusplus
}
#endif

#endif // MIDI_H
