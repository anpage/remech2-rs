#ifndef MIDISEQUENCE_H
#define MIDISEQUENCE_H

#include "decomp.h"
#include "mss.h"
#include "types.h"

class AudioSubsystem;

class MidiSequence {
public:
	MidiSequence(AudioSubsystem* p_subsystem, void* p_data, undefined4 p_size);
	~MidiSequence();

	void SetLoopCount(MechS32 p_loopCount);
	void Start();
	void Stop();
	void SetVolume(MechS32 p_volume);
	undefined IsAnySequencePlaying();
	void ApplyCurrentVolume();

private:
	AudioSubsystem* m_subsystem; // 0x00
	HSEQUENCE m_sequence;        // 0x04
	void* m_data;                // 0x08
	undefined4 m_size;           // 0x0c
	MechS32 m_volume;            // 0x10
	MechS32 m_loopCount;         // 0x14
};

#endif // MIDISEQUENCE_H
