#ifndef SPEECHLINE_H
#define SPEECHLINE_H

#include "types.h"

// A voice message: the sound resource and the text shown without digital sound.
typedef struct SpeechLine {
	int32_t m_id; // a sound resource, or 0
	char* m_text;
	void* m_data; // sound data on the heap, played instead of m_id
} SpeechLine;

#endif // SPEECHLINE_H
