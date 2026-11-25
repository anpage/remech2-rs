#ifndef AUDIO_H
#define AUDIO_H

#include "mss.h"
#include "soundconfig.h"
#include "types.h"

// The functions and globals of audio.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_cdTrack;
	extern SoundConfig g_soundConfig;
	extern SoundConfig* g_mw2SndCfgData;
	extern int32_t g_audioPaused;
	extern int32_t g_musicStarted;
	extern int32_t g_nextEngageCheck;

	int32_t GetSoundSetting(int32_t p_setting);
	void PreviewSoundSetting(int32_t p_setting, int32_t p_value);
	void SetSoundSetting(int32_t p_setting, int32_t p_value);
	void RestoreSoundSetting(int32_t p_setting);
	void StartMissionMusic(void);
	void PauseMusic(void);
	void ResumeMusic(void);
	void StopMusic(void);
	void LoopCdMusic(void);
	int32_t FirstAudio(void);
	void DoAudio(void);
	void ShutdownAudio(void);
	void PauseAudio(void);
	void ResumeAudio(void);
	HDIGDRIVER OpenDigitalDriver(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_H
