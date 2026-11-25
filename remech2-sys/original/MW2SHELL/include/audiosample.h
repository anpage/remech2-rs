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

	void SetFade(int32_t p_initFadeRate, int32_t p_maxFade, int32_t p_startVolume, int32_t p_endVolume);
	void DoFade();
	void EnableLoop();
	void Start();
	void PlayAndWait();
	void Stop();
	undefined IsPlaying();
	void SetVolume(int32_t p_volume);

private:
	ShellAudioSample* m_impl; // NULL when digital audio is off or the sample couldn't be loaded
	void* m_data;             // the sound file, freed with the sample
};

#endif // AUDIOSAMPLE_H
