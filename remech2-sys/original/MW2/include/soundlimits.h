#ifndef SOUNDLIMITS_H
#define SOUNDLIMITS_H

#include "decomp.h"
#include "types.h"

// An entry of the sound table InitSoundInfo unpacks into g_soundInfo.
// SIZE 0x08
typedef struct SoundTableEntry {
	MechS16 m_id;       // 0x00 — the sound resource
	MechS16 m_priority; // 0x02
	MechS16 m_maxCount; // 0x04
	MechS16 m_rate;     // 0x06
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
