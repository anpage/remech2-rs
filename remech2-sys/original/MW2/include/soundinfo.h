#ifndef SOUNDINFO_H
#define SOUNDINFO_H

#include "types.h"

// The playback limits of one sound resource, from the table InitSoundInfo unpacks; -1 is unset.
typedef struct SoundInfo {
	int16_t m_priority;
	int16_t m_maxCount; // how many may play at once
	int16_t m_rate;     // the playback rate
	int16_t m_count;    // how many play now
} SoundInfo;

#endif // SOUNDINFO_H
