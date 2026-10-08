#ifndef AUDIOSUBSYSTEM_H
#define AUDIOSUBSYSTEM_H

#include "decomp.h"
#include "shellaudio.h"
#include "types.h"

class AudioSubsystem {
public:
	AudioSubsystem();
	~AudioSubsystem();

	void ApplyMidiVolume();

	friend class MidiSequence;
	friend class AudioSample;

private:
	ShellAudioSubsystem* m_impl;
};

#endif // AUDIOSUBSYSTEM_H
