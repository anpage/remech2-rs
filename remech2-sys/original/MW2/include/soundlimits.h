#ifndef SOUNDLIMITS_H
#define SOUNDLIMITS_H

#include "types.h"

// An entry of the sound table InitSoundInfo unpacks into g_soundInfo.
typedef struct SoundTableEntry {
	int16_t m_id; // the sound resource
	int16_t m_priority;
	int16_t m_maxCount;
	int16_t m_rate;
} SoundTableEntry;

// The functions and globals of soundlimits.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern SoundTableEntry g_soundTable[0x8c];

	void InitSoundInfo(void);

#ifdef __cplusplus
}
#endif

#endif // SOUNDLIMITS_H
