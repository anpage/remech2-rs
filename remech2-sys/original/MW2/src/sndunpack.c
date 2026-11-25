/* The sound-block decoder.

   The decoder keeps its frame, its delta table and the upsampling's output in these globals
   between calls, and reads what earlier frames left there: a frame that repeats the previous
   one, and the samples past a short frame's end that the upsampling interpolates towards. */
#include "sndunpack.h"

#include "portable.h"
#include "types.h"

#include <string.h>

#define FRAME_SIZE 0x400

uint8_t g_soundUpsampleBuffer[0x400] = {0};
uint8_t g_soundFrame[0x401] = {0};
uint8_t g_soundDeltas[0x43] = {0};

enum SoundCoding {
	c_codingSilence,
	c_codingRepeat,
	c_coding1Bit,
	c_coding2Bit,
	c_coding4Bit,
	c_codingRaw
};

// The delta table's entries, dwords at an odd address.
static int32_t GetDelta(uint32_t p_index)
{
	const uint8_t* entry = &g_soundDeltas[p_index * 4];

	return PortableS32(
		entry[0] | ((uint32_t) entry[1] << 8) | ((uint32_t) entry[2] << 16) | ((uint32_t) entry[3] << 24)
	);
}

// Reads the table that follows a frame's code byte: each byte b is the delta 2b - 0x80.
static const uint8_t* ReadDeltas(const uint8_t* p_src, uint32_t p_count)
{
	uint32_t i;

	for (i = 0; i < p_count; i++) {
		uint32_t delta = ((uint32_t) *p_src++ << 1) - 0x80;
		uint8_t* entry = &g_soundDeltas[i * 4];

		entry[0] = (uint8_t) delta;
		entry[1] = (uint8_t) (delta >> 8);
		entry[2] = (uint8_t) (delta >> 16);
		entry[3] = (uint8_t) (delta >> 24);
	}

	return p_src;
}

// Decodes indices of p_bits bits, lowest first, into the frame until it holds p_count samples
// (whole bytes at a time, at least one), each delta added to *p_value (wrapping, from the
// caller's state) and clamped to -127..127.
static const uint8_t* DecodeDeltas(const uint8_t* p_src, uint32_t p_count, uint32_t p_bits, int32_t* p_value)
{
	uint32_t mask = (1 << p_bits) - 1;
	uint32_t sample = 0;
	uint32_t bits;
	uint32_t i;

	do {
		bits = *p_src++;
		for (i = 0; i < 8 / p_bits; i++) {
			int32_t value = PortableS32((uint32_t) *p_value + (uint32_t) GetDelta(bits & mask));

			if (value > 0x7f) {
				value = 0x7f;
			}
			else if (value < -0x7f) {
				value = -0x7f;
			}

			*p_value = value;
			g_soundFrame[sample++] = (uint8_t) (value + 0x80);
			bits >>= p_bits;
		}
	} while (sample < p_count);

	return p_src;
}

// UpsampleSoundFrame2: upsamples the frame twice in place, interpolating linearly through the scratch
// buffer, to p_size samples. The last pair reads the sample past the decoded ones.
static void Upsample2(uint32_t p_size)
{
	uint8_t* frame = g_soundFrame;
	uint32_t i = 0;

	do {
		uint32_t a = frame[i >> 1];
		uint32_t b = frame[(i >> 1) + 1];

		g_soundUpsampleBuffer[i] = (uint8_t) a;
		g_soundUpsampleBuffer[i + 1] = (uint8_t) ((a + b) >> 1);
		i += 2;
	} while (i < p_size - 1);

	memcpy(frame, g_soundUpsampleBuffer, p_size);
}

// UpsampleSoundFrame4: the same four times.
static void Upsample4(uint32_t p_size)
{
	uint8_t* frame = g_soundFrame;
	uint32_t i = 0;

	do {
		uint32_t a = frame[i >> 2];
		uint32_t b = frame[(i >> 2) + 1];
		uint32_t middle = (a + b) >> 1;

		g_soundUpsampleBuffer[i] = (uint8_t) a;
		g_soundUpsampleBuffer[i + 1] = (uint8_t) ((middle + a) >> 1);
		g_soundUpsampleBuffer[i + 2] = (uint8_t) middle;
		g_soundUpsampleBuffer[i + 3] = (uint8_t) ((middle + b) >> 1);
		i += 4;
	} while (i < p_size - 1);

	memcpy(frame, g_soundUpsampleBuffer, p_size);
}

// p_count is at least 1, and p_frameSize 1 to 0x400; a raw frame has at least one sample before
// the upsampling (the original's loops run 2^32 times otherwise). An unknown coding ends the
// decoding with NULL, leaving *p_state at the previous frame's value.
uint8_t* DecodeSoundFrames(uint8_t* p_src, uint8_t* p_dst, uint32_t p_count, uint32_t p_frameSize, int32_t* p_state)
{
	const uint8_t* src = p_src;

	PORTABLE_ASSERT(p_count != 0);
	PORTABLE_ASSERT(p_frameSize != 0 && p_frameSize <= FRAME_SIZE);
	do {
		uint32_t upsampling = *src >> 6;
		uint32_t coding = *src & 0xf;
		uint32_t samples = p_frameSize;
		int32_t value = *p_state;

		if (upsampling == 1) {
			samples >>= 1;
		}
		else if (upsampling == 2) {
			samples >>= 2;
		}

		src++;
		switch (coding) {
		case c_codingSilence:
			memset(g_soundFrame, 0x80, samples);
			value = 0;
			break;
		case c_codingRepeat:
			break;
		case c_coding1Bit:
			src = DecodeDeltas(ReadDeltas(src, 2), samples, 1, &value);
			break;
		case c_coding2Bit:
			src = DecodeDeltas(ReadDeltas(src, 4), samples, 2, &value);
			break;
		case c_coding4Bit:
			src = DecodeDeltas(ReadDeltas(src, 16), samples, 4, &value);
			break;
		case c_codingRaw:
			PORTABLE_ASSERT(samples != 0);
			memcpy(g_soundFrame, src, samples);
			src += samples;
			value = g_soundFrame[samples - 1] - 0x80;
			break;
		default:
			return NULL;
		}

		if (upsampling == 1) {
			Upsample2(p_frameSize);
		}
		else if (upsampling == 2) {
			Upsample4(p_frameSize);
		}

		*p_state = value;
		memcpy(p_dst, g_soundFrame, p_frameSize);
		p_dst += p_frameSize;
	} while (--p_count);

	return (uint8_t*) src;
}
