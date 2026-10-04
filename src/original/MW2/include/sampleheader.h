#ifndef SAMPLEHEADER_H
#define SAMPLEHEADER_H

#include "decomp.h"
#include "types.h"

#pragma pack(push, 1)

// The header of a sound resource ("SNDS"): the "SFLX" tag, the sample count and the bytes per
// sample. The sample data follows.
// SIZE 0x0e
typedef struct SampleHeader {
	MechChar m_tag[4];   // 0x00 — "SFLX"
	undefined4 m_size;   // 0x04
	MechU32 m_length;    // 0x08 — in samples
	MechU16 m_frameSize; // 0x0c — bytes per sample
} SampleHeader;

#pragma pack(pop)

#endif // SAMPLEHEADER_H
