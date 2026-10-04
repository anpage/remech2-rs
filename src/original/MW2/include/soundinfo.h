#ifndef SOUNDINFO_H
#define SOUNDINFO_H

#include "decomp.h"
#include "types.h"

// The playback limits of one sound resource, from the table InitSoundInfo unpacks; -1 is unset.
// SIZE 0x08
typedef struct SoundInfo {
	MechS16 m_priority; // 0x00
	MechS16 m_maxCount; // 0x02 — how many may play at once
	MechS16 m_rate;     // 0x04 — the playback rate
	MechS16 m_count;    // 0x06 — how many play now
} SoundInfo;

#endif // SOUNDINFO_H
