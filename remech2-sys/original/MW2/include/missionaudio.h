#ifndef MISSIONAUDIO_H
#define MISSIONAUDIO_H

#include "types.h"

// A project file entry: a sound file name, chained in its hash bucket.
typedef struct ProjectFileEntry {
	char m_name[0x10];               // 0x00
	struct ProjectFileEntry* m_next; // 0x10
} ProjectFileEntry;

// The functions of missionaudio.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_soundFileCount;
	extern ProjectFileEntry g_soundFileEntries[200];
	extern ProjectFileEntry* g_soundFileTable[0x65];
	extern char g_soundFileDir[0x100];

	void CollectMissionAudio(void);
	void* ReadSoundFile(char* p_name);

#ifdef __cplusplus
}
#endif

#endif // MISSIONAUDIO_H
