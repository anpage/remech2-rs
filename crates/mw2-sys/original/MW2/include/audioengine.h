#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include "decomp.h"
#include "mss.h"
#include "sampleheader.h"
#include "types.h"

// The digital sound effects: up to 16 Miles samples, each streamed from its sound resource
// through two 8K halves of a 16K buffer. Slots 8-15 also hold the looping ambient sounds.
// SIZE 0x508
typedef struct AudioEngine {
	HDIGDRIVER m_driver;        // 0x000
	HSAMPLE m_samples[16];      // 0x004
	MechU32 m_numSamples;       // 0x044
	MechS32 m_playing[16];      // 0x048 — cleared by the end-of-sample callback
	SampleHeader m_headers[16]; // 0x088
	MechU32 m_remaining[16];    // 0x168 — samples not yet loaded
	MechU32 m_frameSizes[16];   // 0x1a8
	MechU32 m_bufferFrames[16]; // 0x1e8 — the samples that fit in half a buffer
	MechS32 m_chunkFrames[16];  // 0x228 — the samples of the next load
	MechU32 m_bufferSizes[16];  // 0x268 — the bytes of half a buffer
	MechU8* m_buffers[16][2];   // 0x2a8 — the halves of one 16K block
	MechS32 m_decodeStates[16]; // 0x328
	MechU8* m_data[16];         // 0x368 — the next sample to load
	MechS32 m_startTimes[16];   // 0x3a8 — clock ticks
	MechS32 m_bearings[16];     // 0x3e8 — for the pan when m_pans is -1
	MechS32 m_pans[16];         // 0x428
	MechS32 m_volumes[16];      // 0x468
	MechS32 m_ids[16];          // 0x4a8 — the resource played
	MechU16 m_flags[16];        // 0x4e8 — the low byte is the priority
} AudioEngine;

#endif // AUDIOENGINE_H
