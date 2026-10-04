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

	extern MechS32 g_cdTrack;
	extern SoundConfig g_soundConfig;
	extern SoundConfig* g_mw2SndCfgData;
	extern MechS32 g_audioPaused;

	MechS32 GetSoundSetting(MechS32 p_setting);
	void PreviewSoundSetting(MechS32 p_setting, MechS32 p_value);
	void SetSoundSetting(MechS32 p_setting, MechS32 p_value);
	void RestoreSoundSetting(MechS32 p_setting);
	void StartMissionMusic(void);
	void PauseMusic(void);
	void ResumeMusic(void);
	void StopMusic(void);
	void LoopCdMusic(void);
	MechS32 FirstAudio(void);
	void DoAudio(void);
	void ShutdownAudio(void);
	void PauseAudio(void);
	void ResumeAudio(void);
	HDIGDRIVER OpenDigitalDriver(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_H
