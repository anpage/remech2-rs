#ifndef SPEECHENTRY_H
#define SPEECHENTRY_H

#include "decomp.h"
#include "types.h"

#pragma pack(push, 1)

// A queued voice message (QueueSpeech): a line and an optional suffix, played one after the
// other, with their joined text.
// SIZE 0x53
typedef struct SpeechEntry {
	MechS32 m_used;             // 0x00
	MechS32 m_id;               // 0x04
	MechS32 m_suffixId;         // 0x08
	void* m_data;               // 0x0c
	void* m_suffixData;         // 0x10
	MechS32 m_deadline;         // 0x14 — the clock tick a text-only message ends
	MechS32 m_priority;         // 0x18
	MechChar m_text[0x33];      // 0x1c
	struct SpeechEntry* m_next; // 0x4f
} SpeechEntry;

#pragma pack(pop)

#endif // SPEECHENTRY_H
