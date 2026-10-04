#ifndef SOUNDFX_H
#define SOUNDFX_H

#include "ambientsound.h"
#include "audioengine.h"
#include "decomp.h"
#include "mss.h"
#include "soundinfo.h"
#include "types.h"

// The functions and globals of soundfx.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_temperature;
	extern MechS32 g_hostileAtmosphere;
	extern MechS32 g_sampleRates[10];
	extern AudioEngine* g_audioEngine;
	extern SoundInfo g_soundInfo[1200];

	MechS32 InitializeDigitalAudio(MechU32 p_numSamples);
	void ShutdownDigitalAudio(void);
	void ServeSamples(void);
	MechS32 PlaySample(
		MechS32 p_delay,
		MechS32 p_bearing,
		MechS32 p_id,
		void* p_data,
		MechU32 p_volume,
		MechS32 p_volumeScale,
		MechS32 p_pan,
		MechS32 p_rate,
		MechS32* p_userData,
		MechU16 p_flags
	);
	MechS32 StartSample(MechS32 p_id, void* p_data, MechU16 p_flags, MechS16 p_slot, MechS32* p_userData);
	void AILCALLBACK SampleEosCallback(HSAMPLE p_sample);
	void PlayEffectsVolumeTest(void);
	MechS32 PlaySoundOnce(MechS32 p_id, MechU32 p_volume, MechS32 p_pan, MechS32 p_rate);
	MechS32 PlaySoundRandomRate(
		MechS32 p_delay,
		MechS32 p_bearing,
		MechS32 p_id,
		MechU32 p_volume,
		MechS32 p_pan,
		MechU16 p_flags
	);
	MechS32 PlayPositionalSound(MechS32 p_dx, MechS32 p_dy, MechS32 p_dz, MechS32 p_sound, MechS32 p_half);
	MechS32 PlaySoundEffect(MechS32 p_id, MechU32 p_volume, MechS32 p_pan, MechS32 p_rate, MechU16 p_flags);
	MechS32 PlayDelayedSound(
		MechS32 p_delay,
		MechS32 p_bearing,
		MechS32 p_id,
		MechU32 p_volume,
		MechS32 p_pan,
		MechU16 p_flags
	);
	MechS32 PlaySoundAt(MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_sound, MechS32 p_half);
	MechS32 CalculateSamplePan(MechS32 p_bearing);
	MechS32 RandomSampleRate(void);
	void StopSamples(MechS32 p_all);
	void UpdateAmbientSound(AmbientSound* p_sound);
	void StopAmbientSound(AmbientSound* p_sound);
	MechS32 GetDistanceVolume(MechS32 p_distance);

#ifdef __cplusplus
}
#endif

#endif // SOUNDFX_H
