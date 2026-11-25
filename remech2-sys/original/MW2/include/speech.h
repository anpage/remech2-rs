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
	extern int32_t g_speechLocked;
	extern SpeechEntry g_speechEntries[8];

	int32_t QueueSpeech(SpeechLine* p_line, SpeechLine* p_suffix, int32_t p_priority);
	void AdvanceSpeechQueue(void);
	int32_t StartSpeech(SpeechEntry* p_entry);
	void PlayVoiceVolumeTest(void);
	void FlushSpeechQueue(int32_t p_keep);
	SpeechEntry* FreeSpeechEntry(SpeechEntry* p_entry);
	void StopSpeech(void);
	void ResumeSpeech(void);
	void SayLancemateReport(int32_t p_message, int32_t p_slot);
	void SayFormation(int32_t p_formation);
	void PlayCockpitSound(int32_t p_message, int32_t p_engage);
	void SayCriticalHit(int32_t p_part);
	int32_t QueueSpeechLine(SpeechLine* p_line);

#ifdef __cplusplus
}
#endif

#endif // SPEECH_H
