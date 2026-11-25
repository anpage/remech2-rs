#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include "mss.h"
#include "sampleheader.h"
#include "types.h"

// The digital sound effects: up to 16 Miles samples, each streamed from its sound resource
// through two 8K halves of a 16K buffer. Slots 8-15 also hold the looping ambient sounds.
typedef struct AudioEngine {
	HDIGDRIVER m_driver;
	HSAMPLE m_samples[16];
	uint32_t m_numSamples;
	int32_t m_playing[16]; // cleared by the end-of-sample callback
	SampleHeader m_headers[16];
	uint32_t m_remaining[16]; // samples not yet loaded
	uint32_t m_frameSizes[16];
	uint32_t m_bufferFrames[16]; // the samples that fit in half a buffer
	int32_t m_chunkFrames[16];   // the samples of the next load
	uint32_t m_bufferSizes[16];  // the bytes of half a buffer
	uint8_t* m_buffers[16][2];   // the halves of one 16K block
	int32_t m_decodeStates[16];
	uint8_t* m_data[16];      // the next sample to load
	int32_t m_startTimes[16]; // clock ticks
	int32_t m_bearings[16];   // for the pan when m_pans is -1
	int32_t m_pans[16];
	int32_t m_volumes[16];
	int32_t m_ids[16];    // the resource played
	uint16_t m_flags[16]; // the low byte is the priority
} AudioEngine;

#endif // AUDIOENGINE_H
