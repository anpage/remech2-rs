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

	extern int32_t g_temperature;
	extern int32_t g_hostileAtmosphere;
	extern int32_t g_sampleRates[10];
	extern AudioEngine* g_audioEngine;
	extern SoundInfo g_soundInfo[1200];
	extern char g_sampleTag[5];

	int32_t InitializeDigitalAudio(uint32_t p_numSamples);
	void ShutdownDigitalAudio(void);
	void ServeSamples(void);
	int32_t PlaySample(
		int32_t p_delay,
		int32_t p_bearing,
		int32_t p_id,
		void* p_data,
		uint32_t p_volume,
		int32_t p_volumeScale,
		int32_t p_pan,
		int32_t p_rate,
		int32_t* p_userData,
		uint16_t p_flags
	);
	int32_t StartSample(int32_t p_id, void* p_data, uint16_t p_flags, int16_t p_slot, int32_t* p_userData);
	void AILCALLBACK SampleEosCallback(HSAMPLE p_sample);
	void PlayEffectsVolumeTest(void);
	int32_t PlaySoundOnce(int32_t p_id, uint32_t p_volume, int32_t p_pan, int32_t p_rate);
	int32_t PlaySoundRandomRate(
		int32_t p_delay,
		int32_t p_bearing,
		int32_t p_id,
		uint32_t p_volume,
		int32_t p_pan,
		uint16_t p_flags
	);
	int32_t PlayPositionalSound(int32_t p_dx, int32_t p_dy, int32_t p_dz, int32_t p_sound, int32_t p_half);
	int32_t PlaySoundEffect(int32_t p_id, uint32_t p_volume, int32_t p_pan, int32_t p_rate, uint16_t p_flags);
	int32_t PlayDelayedSound(
		int32_t p_delay,
		int32_t p_bearing,
		int32_t p_id,
		uint32_t p_volume,
		int32_t p_pan,
		uint16_t p_flags
	);
	int32_t PlaySoundAt(int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_sound, int32_t p_half);
	int32_t CalculateSamplePan(int32_t p_bearing);
	int32_t RandomSampleRate(void);
	void StopSamples(int32_t p_all);
	void UpdateAmbientSound(AmbientSound* p_sound);
	void StopAmbientSound(AmbientSound* p_sound);
	int32_t GetDistanceVolume(int32_t p_distance);

#ifdef __cplusplus
}
#endif

#endif // SOUNDFX_H
