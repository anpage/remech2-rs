#ifndef AUDIOSAMPLE_H
#define AUDIOSAMPLE_H

#include "decomp.h"
#include "mss.h"
#include "types.h"

class AudioSubsystem;

// SIZE 0x2c
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
	void SetLoopCount(MechS32 p_loopCount);

private:
	AudioSubsystem* m_subsystem; // 0x00
	void* m_data;                // 0x04
	undefined4 m_size;           // 0x08
	HSAMPLE m_sample;            // 0x0c
	MechS32 m_volume;            // 0x10
	MechS32 m_loopCount;         // 0x14
	MechS32 m_initFadeRate;      // 0x18
	MechS32 m_fadeRate;          // 0x1c
	MechS32 m_maxFade;           // 0x20
	MechS32 m_startVolume;       // 0x24
	MechS32 m_endVolume;         // 0x28
};

#endif // AUDIOSAMPLE_H
