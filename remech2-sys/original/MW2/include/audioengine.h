#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include "decomp.h"
#include "mss.h"
#include "sampleheader.h"
#include "types.h"

// The digital sound effects: up to 16 Miles samples, each streamed from its sound resource
// through two 8K halves of a 16K buffer. Slots 8-15 also hold the looping ambient sounds.
typedef struct AudioEngine {
	HDIGDRIVER m_driver;         // 0x000
	HSAMPLE m_samples[16];       // 0x004
	uint32_t m_numSamples;       // 0x044
	int32_t m_playing[16];       // 0x048 — cleared by the end-of-sample callback
	SampleHeader m_headers[16];  // 0x088
	uint32_t m_remaining[16];    // 0x168 — samples not yet loaded
	uint32_t m_frameSizes[16];   // 0x1a8
	uint32_t m_bufferFrames[16]; // 0x1e8 — the samples that fit in half a buffer
	int32_t m_chunkFrames[16];   // 0x228 — the samples of the next load
	uint32_t m_bufferSizes[16];  // 0x268 — the bytes of half a buffer
	uint8_t* m_buffers[16][2];   // 0x2a8 — the halves of one 16K block
	int32_t m_decodeStates[16];  // 0x328
	uint8_t* m_data[16];         // 0x368 — the next sample to load
	int32_t m_startTimes[16];    // 0x3a8 — clock ticks
	int32_t m_bearings[16];      // 0x3e8 — for the pan when m_pans is -1
	int32_t m_pans[16];          // 0x428
	int32_t m_volumes[16];       // 0x468
	int32_t m_ids[16];           // 0x4a8 — the resource played
	uint16_t m_flags[16];        // 0x4e8 — the low byte is the priority
} AudioEngine;

#endif // AUDIOENGINE_H
