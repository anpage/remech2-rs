#ifndef AUDIOSAMPLE_H
#define AUDIOSAMPLE_H

#include "decomp.h"
#include "shellaudio.h"
#include "types.h"

class AudioSubsystem;

class AudioSample {
public:
	AudioSample(AudioSubsystem* p_subsystem, void* p_data, undefined4 p_size);
	~AudioSample();

	void SetFade(MechS32 p_initFadeRate, MechS32 p_maxFade, MechS32 p_startVolume, MechS32 p_endVolume);
	void DoFade();
	void EnableLoop();
	void Start();
	void PlayAndWait();
	void Stop();
	undefined IsPlaying();
	void SetVolume(MechS32 p_volume);

private:
	ShellAudioSample* m_impl; // NULL when digital audio is off or the sample couldn't be loaded
	void* m_data;             // the sound file, freed with the sample
};

#endif // AUDIOSAMPLE_H
