#ifndef SAMPLEHEADER_H
#define SAMPLEHEADER_H

#include "types.h"

#pragma pack(push, 1)

// The header of a sound resource ("SNDS"): the "SFLX" tag, the sample count and the bytes per
// sample. The sample data follows.
typedef struct SampleHeader {
	char m_tag[4]; // "SFLX"
	uint32_t m_size;
	uint32_t m_length;    // in samples
	uint16_t m_frameSize; // bytes per sample
} SampleHeader;

#pragma pack(pop)

#endif // SAMPLEHEADER_H
