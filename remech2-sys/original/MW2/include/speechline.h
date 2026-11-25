#ifndef SPEECHLINE_H
#define SPEECHLINE_H

#include "decomp.h"
#include "types.h"

// A voice message: the sound resource and the text shown without digital sound.
typedef struct SpeechLine {
	int32_t m_id; // 0x00 — a sound resource, or 0
	char* m_text; // 0x04
	void* m_data; // 0x08 — sound data on the heap, played instead of m_id
} SpeechLine;

#endif // SPEECHLINE_H
