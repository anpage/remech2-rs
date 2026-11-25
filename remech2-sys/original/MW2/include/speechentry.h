#ifndef SPEECHENTRY_H
#define SPEECHENTRY_H

#include "types.h"

#pragma pack(push, 1)

// A queued voice message (QueueSpeech): a line and an optional suffix, played one after the
// other, with their joined text.
typedef struct SpeechEntry {
	int32_t m_used;
	int32_t m_id;
	int32_t m_suffixId;
	void* m_data;
	void* m_suffixData;
	int32_t m_deadline; // the clock tick a text-only message ends
	int32_t m_priority;
	char m_text[0x33];
	struct SpeechEntry* m_next;
} SpeechEntry;

#pragma pack(pop)

#endif // SPEECHENTRY_H
