/* The mission's MIDI music: up to eight Miles sequences loaded from "XMID" resources, and a
   locked MIDI channel that holds a note whose pitch and volume follow the local mech. */
#include "midi.h"

#include "audio.h"
#include "decomp.h"
#include "fixedmul.h"
#include "loadres.h"
#include "mech.h"
#include "mss.h"
#include "mw2prj.h"
#include "players.h"
#include "simmain.h"
#include "types.h"

// The channel AIL_lock_channel gave the note (0-based), or -1.
// GLOBAL: MW2 0x100a3948
MechS8 g_lockedChannel = -1;

// GLOBAL: MW2 0x100a394c
HMDIDRIVER g_midiDriver = NULL;

// Set by ShutdownMidi: the sequences are gone for good.
// GLOBAL: MW2 0x100a3950
MechS32 g_midiShutDown = 0;

// GLOBAL: MW2 0x100a3954
MechS32 g_midiInitialized = 0;

// The resource ids of the sequences' data, or 0.
// GLOBAL: MW2 0x1010b5e0
MechS16 g_midiSequenceIds[8];

// The sequence handles, or -1.
// GLOBAL: MW2 0x1010b5f0
HSEQUENCE g_midiSequences[8];

// FUNCTION: MW2 0x10021460
void StopMidiSequences(void)
{
	HSEQUENCE sequence;
	MechS32 i;

	if (g_midiShutDown) {
		return;
	}

	for (i = 0; i < 8; i++) {
		if (g_midiSequences[i] != (HSEQUENCE) -1) {
			sequence = g_midiSequences[i];
			if (AIL_sequence_status(sequence) == SEQ_PLAYING) {
				AIL_stop_sequence(sequence);
			}

			AIL_release_sequence_handle(sequence);
			g_midiSequences[i] = (HSEQUENCE) -1;
		}

		if (g_midiSequenceIds[i]) {
			UnlockCachedResource(g_midiSequenceIds[i], g_resourceTypeTags[c_resTagXmid]);
			g_midiSequenceIds[i] = 0;
		}
	}
}

// FUNCTION: MW2 0x1002152b
void PauseMidiSequences(void)
{
	HSEQUENCE sequence;
	MechS32 i;

	if (g_midiShutDown) {
		return;
	}

	for (i = 0; i < 8; i++) {
		if (g_midiSequences[i] != (HSEQUENCE) -1) {
			sequence = g_midiSequences[i];
			if (AIL_sequence_status(sequence) == SEQ_PLAYING) {
				AIL_stop_sequence(sequence);
			}
		}
	}
}

// FUNCTION: MW2 0x100215a4
void ResumeMidiSequences(void)
{
	HSEQUENCE sequence;
	MechS32 i;

	if (g_midiShutDown) {
		return;
	}

	for (i = 0; i < 8; i++) {
		if (g_midiSequences[i] != (HSEQUENCE) -1) {
			sequence = g_midiSequences[i];
			if (AIL_sequence_status(sequence) == SEQ_STOPPED) {
				AIL_resume_sequence(sequence);
			}
		}
	}
}

// Returns 1, -1 when already initialized or -5 when no MIDI device opens.
// FUNCTION: MW2 0x1002161d
MechS32 InitializeMidi(void)
{
	MechS32 i;

	if (g_midiInitialized) {
		return -1;
	}

	for (i = 0; i < 8; i++) {
		g_midiSequences[i] = (HSEQUENCE) -1;
		g_midiSequenceIds[i] = 0;
	}

	AIL_set_preference(9, 12);
	g_midiDriver = OpenMidiDriver();
	if (g_midiDriver == NULL) {
		return -5;
	}

	SetMidiVolume(FixedMul16(g_soundConfig.m_midiVolume, 0x7f));
	g_midiInitialized = 1;
	return 1;
}

// Releases the sequences that are done and starts sequence p_sequenceNum of the "XMID" resource
// p_id in a free slot. Returns 1, 0 when there is nothing to play, -8 when no slot is free or
// -10 when Miles has no sequence handle. The slot's resource id isn't recorded.
// Stack-slot permutation: slot and i.
// FUNCTION: MW2 0x100216d3
MechS32 PlayMidiSequence(undefined4 p_unk0x00, MechS16 p_id, MechS16 p_sequenceNum)
{
	HSEQUENCE sequence;
	MechS32 slot;
	MechS32 i;
	void* data;

	sequence = (HSEQUENCE) -1;
	slot = -1;
	if (p_id < 1) {
		return 0;
	}

	for (i = 0; i < 8; i++) {
		sequence = g_midiSequences[i];
		if (sequence != (HSEQUENCE) -1 && AIL_sequence_status(sequence) == SEQ_DONE) {
			AIL_release_sequence_handle(sequence);
			g_midiSequences[i] = (HSEQUENCE) -1;
			sequence = g_midiSequences[i];
			UnlockCachedResource(g_midiSequenceIds[i], g_resourceTypeTags[c_resTagXmid]);
			g_midiSequenceIds[i] = 0;
		}

		if (sequence == (HSEQUENCE) -1) {
			slot = i;
		}
	}

	if (slot == -1) {
		return -8;
	}

	data = LoadCachedResource(p_unk0x00, p_id, g_resourceTypeTags[c_resTagXmid], 0);
	if (data && g_midiDriver) {
		sequence = AIL_allocate_sequence_handle(g_midiDriver);
		if (sequence == NULL) {
			UnlockCachedResource(p_id, g_resourceTypeTags[c_resTagXmid]);
			return -10;
		}

		g_midiSequences[slot] = sequence;
		AIL_init_sequence(sequence, data, p_sequenceNum);
		AIL_set_sequence_volume(sequence, FixedMul16(g_soundConfig.m_midiVolume, 0x7f), 0);
		AIL_start_sequence(sequence);
		return 1;
	}

	return 0;
}

// FUNCTION: MW2 0x1002187c
void ShutdownMidi(void)
{
	if (!g_midiInitialized) {
		return;
	}

	if (!g_midiShutDown) {
		StopMidiSequences();
	}

	g_midiShutDown = 1;
	g_midiInitialized = 0;
}

// Returns whether a sequence is playing; when none is, releases them all.
// Stack-slot permutation: i and playing.
// FUNCTION: MW2 0x100218bf
MechS16 AnyMidiPlaying(void)
{
	MechS32 i;
	MechS32 playing;

	playing = 0;
	if (g_midiShutDown) {
		return 0;
	}

	for (i = 0; i < 8; i++) {
		if (g_midiSequences[i] != (HSEQUENCE) -1 && AIL_sequence_status(g_midiSequences[i]) == SEQ_PLAYING) {
			playing = 1;
			break;
		}
	}

	if (!playing) {
		StopMidiSequences();
	}

	return playing;
}

// Fades the playing sequences to p_volume (0 to 127) over a second.
// FUNCTION: MW2 0x10021956
void SetMidiVolume(MechS16 p_volume)
{
	MechS32 i;

	if (g_midiShutDown) {
		return;
	}

	if (AnyMidiPlaying()) {
		for (i = 0; i < 8; i++) {
			if (g_midiSequences[i] != (HSEQUENCE) -1 && AIL_sequence_status(g_midiSequences[i]) == SEQ_PLAYING) {
				AIL_set_sequence_volume(g_midiSequences[i], p_volume, 1000);
			}
		}
	}
}

// FUNCTION: MW2 0x100219ea
void ApplyMidiVolume(void)
{
}

// Has the same parameters as PlayDelayedSound, which plays the sound itself when this returns
// 0 or less.
// FUNCTION: MW2 0x100219f5
MechS32 FUN_100219f5(MechS32 p_delay, MechS32 p_bearing, MechS32 p_id, MechU32 p_volume, MechS32 p_pan, MechS32 p_flags)
{
	return 0;
}

// Locks a MIDI channel and starts the held note on it: program 3, pitch bend 0x1800, centered,
// silent until UpdateEngineNote sets its volume.
// FUNCTION: MW2 0x10021a07
void StartEngineNote(void)
{
	if (g_midiDriver == NULL) {
		return;
	}

	if (g_lockedChannel >= 0) {
		return;
	}

	if (g_midiInitialized) {
		g_lockedChannel = AIL_lock_channel(g_midiDriver);
		if (!g_lockedChannel) {
			return;
		}

		g_lockedChannel--;
		AIL_send_channel_voice_message(g_midiDriver, NULL, g_lockedChannel | 0xb0, 0x72, 0);
		AIL_send_channel_voice_message(g_midiDriver, NULL, g_lockedChannel | 0xc0, 3, 0);
		AIL_send_channel_voice_message(g_midiDriver, NULL, g_lockedChannel | 0xe0, 0, 0x30);
		AIL_send_channel_voice_message(g_midiDriver, NULL, g_lockedChannel | 0xb0, 10, 0x40);
		AIL_send_channel_voice_message(g_midiDriver, NULL, g_lockedChannel | 0xb0, 7, 0);
		AIL_send_channel_voice_message(g_midiDriver, NULL, g_lockedChannel | 0x90, 0x26, 0x7f);
	}
}

// Bends the held note by p_pitch and sets its volume from the effects volume, while the local
// mech's power state is above 1.
// FUNCTION: MW2 0x10021b2a
void UpdateEngineNote(MechU32 p_pitch)
{
	if (g_midiDriver == NULL) {
		return;
	}

	if (g_lockedChannel >= 0) {
		if (g_players[g_localPlayerId]->m_mech->m_powerState <= 1) {
			return;
		}

		p_pitch <<= 2;
		p_pitch += 0x2000;
		AIL_send_channel_voice_message(
			g_midiDriver,
			NULL,
			g_lockedChannel | 0xe0,
			p_pitch & 0x7f,
			(p_pitch & 0x3f00) >> 8
		);
		AIL_send_channel_voice_message(
			g_midiDriver,
			NULL,
			g_lockedChannel | 0xb0,
			7,
			(MechU8) FixedMul16(g_soundConfig.m_effectsVolume, 100)
		);
	}
}

// Ends the held note and unlocks its channel.
// FUNCTION: MW2 0x10021be2
void StopEngineNote(void)
{
	if (g_midiDriver == NULL) {
		return;
	}

	if (g_lockedChannel >= 0) {
		AIL_send_channel_voice_message(g_midiDriver, NULL, g_lockedChannel | 0x80, 0x26, 0x7f);
		AIL_release_channel(g_midiDriver, g_lockedChannel + 1);
		g_lockedChannel = -1;
	}
}

// Silences the held note.
// FUNCTION: MW2 0x10021c49
void MuteEngineNote(void)
{
	if (g_midiDriver == NULL) {
		return;
	}

	if (g_lockedChannel >= 0) {
		AIL_send_channel_voice_message(g_midiDriver, NULL, g_lockedChannel | 0xb0, 7, 0);
	}
}

// Restores the held note's volume from the effects volume.
// FUNCTION: MW2 0x10021c94
void UnmuteEngineNote(void)
{
	if (g_midiDriver == NULL) {
		return;
	}

	if (g_lockedChannel >= 0) {
		AIL_send_channel_voice_message(
			g_midiDriver,
			NULL,
			g_lockedChannel | 0xb0,
			7,
			(MechU8) FixedMul16(g_soundConfig.m_effectsVolume, 100)
		);
	}
}
