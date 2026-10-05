#ifndef MIDISEQUENCE_H
#define MIDISEQUENCE_H

#include "decomp.h"
#include "shellaudio.h"
#include "types.h"

class AudioSubsystem;

class MidiSequence {
public:
	MidiSequence(AudioSubsystem* p_subsystem, void* p_data, undefined4 p_size);
	~MidiSequence();

	void Start();
	void Stop();
	void SetVolume(MechS32 p_volume);
	undefined IsAnySequencePlaying();

private:
	ShellMidiSequence* m_impl; // NULL when MIDI is off or the sequence couldn't be loaded
	void* m_data;              // the XMIDI file, freed with the sequence
};

#endif // MIDISEQUENCE_H
