#ifndef SNDUNPACK_H
#define SNDUNPACK_H

#include "types.h"

// The sound-block decoder (sndunpack.c).
#ifdef __cplusplus
extern "C"
{
#endif

	extern uint8_t g_soundUpsampleBuffer[0x400];
	extern uint8_t g_soundFrame[0x401];
	extern uint8_t g_soundDeltas[0x43];

	uint8_t* DecodeSoundFrames(
		uint8_t* p_src,
		uint8_t* p_dst,
		uint32_t p_count,
		uint32_t p_frameSize,
		int32_t* p_state
	);

#ifdef __cplusplus
}
#endif

#endif // SNDUNPACK_H
