#ifndef SAMPLEHEADER_H
#define SAMPLEHEADER_H

#include "types.h"

#pragma pack(push, 1)

// The header of a sound resource ("SNDS"): the "SFLX" tag, the sample count and the bytes per
// sample. The sample data follows.
typedef struct SampleHeader {
	char m_tag[4];        // 0x00 — "SFLX"
	uint32_t m_size;      // 0x04
	uint32_t m_length;    // 0x08 — in samples
	uint16_t m_frameSize; // 0x0c — bytes per sample
} SampleHeader;

#pragma pack(pop)

#endif // SAMPLEHEADER_H
