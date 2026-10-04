#ifndef AUDIOSUBSYSTEM_H
#define AUDIOSUBSYSTEM_H

#include "decomp.h"
#include "mss.h"
#include "types.h"

class MidiSequence;

#pragma pack(1)
// SIZE 0x15
class AudioSubsystem {
public:
	AudioSubsystem();
	~AudioSubsystem();

	HDIGDRIVER GetDigitalDriver();
	void CloseDigitalDriver();
	undefined IsAnySamplePlaying();
	undefined IsAnySequencePlaying();
	void ApplyMidiVolume();
	MechS32 GetMidiVolume();
	MechS32 GetEffectsVolume();
	void StopMidiSequence();

	friend class MidiSequence;
	friend class VideoSound;

private:
	MechU8 m_milesStarted;               // 0x00
	HMDIDRIVER m_midiDriver;             // 0x01
	HDIGDRIVER m_digitalDriver;          // 0x05
	LPHWAVEOUT m_waveOut;                // 0x09
	MidiSequence* m_currentMidiSequence; // 0x0d
	undefined4 m_playbackRate;           // 0x11 — VideoSound passes it to AIL_minimum_sample_buffer_size
};
#pragma pack()

#endif // AUDIOSUBSYSTEM_H
