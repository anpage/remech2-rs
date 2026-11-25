#ifndef SOUNDINFO_H
#define SOUNDINFO_H

#include "decomp.h"
#include "types.h"

// The playback limits of one sound resource, from the table InitSoundInfo unpacks; -1 is unset.
typedef struct SoundInfo {
	int16_t m_priority; // 0x00
	int16_t m_maxCount; // 0x02 — how many may play at once
	int16_t m_rate;     // 0x04 — the playback rate
	int16_t m_count;    // 0x06 — how many play now
} SoundInfo;

#endif // SOUNDINFO_H
