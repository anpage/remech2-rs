#ifndef SPEECH_H
#define SPEECH_H

#include "decomp.h"
#include "mss.h"
#include "speechentry.h"
#include "speechline.h"
#include "types.h"

// The functions and globals of speech.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern SpeechLine g_slotSpeech[3];
	extern SpeechLine g_lancemateSpeech[11];
	extern SpeechLine g_formationSpeech[7];
	extern SpeechLine g_engageSpeech[3];
	extern SpeechLine g_cockpitSpeech[34];
	extern SpeechLine g_damageSpeech[39];
	extern SpeechEntry* g_speechQueue;
	extern HSAMPLE g_speechSample;
	extern MechS32 g_speechLocked;

	MechS32 QueueSpeech(SpeechLine* p_line, SpeechLine* p_suffix, MechS32 p_priority);
	void AdvanceSpeechQueue(void);
	MechS32 StartSpeech(SpeechEntry* p_entry);
	void PlayVoiceVolumeTest(void);
	void FlushSpeechQueue(MechS32 p_keep);
	SpeechEntry* FreeSpeechEntry(SpeechEntry* p_entry);
	void StopSpeech(void);
	void ResumeSpeech(void);
	void SayLancemateReport(MechS32 p_message, MechS32 p_slot);
	void SayFormation(MechS32 p_formation);
	void PlayCockpitSound(MechS32 p_message, MechS32 p_engage);
	void SayCriticalHit(MechS32 p_part);
	MechS32 QueueSpeechLine(SpeechLine* p_line);

#ifdef __cplusplus
}
#endif

#endif // SPEECH_H
