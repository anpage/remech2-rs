#ifndef MISSIONAUDIO_H
#define MISSIONAUDIO_H

#include "types.h"

// The functions of missionaudio.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void CollectMissionAudio(void);
	void* ReadSoundFile(MechChar* p_name);

#ifdef __cplusplus
}
#endif

#endif // MISSIONAUDIO_H
