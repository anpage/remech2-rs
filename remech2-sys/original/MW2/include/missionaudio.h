#ifndef MISSIONAUDIO_H
#define MISSIONAUDIO_H

#include "types.h"

// A project file entry: a sound file name, chained in its hash bucket.
// SIZE 0x14
typedef struct ProjectFileEntry {
	MechChar m_name[0x10];           // 0x00
	struct ProjectFileEntry* m_next; // 0x10
} ProjectFileEntry;

// The functions of missionaudio.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_soundFileCount;
	extern ProjectFileEntry g_soundFileEntries[200];
	extern ProjectFileEntry* g_soundFileTable[0x65];
	extern MechChar g_soundFileDir[0x100];

	void CollectMissionAudio(void);
	void* ReadSoundFile(MechChar* p_name);

#ifdef __cplusplus
}
#endif

#endif // MISSIONAUDIO_H
