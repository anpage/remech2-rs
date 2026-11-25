#ifndef SNDUNPACK_H
#define SNDUNPACK_H

#include "types.h"

// The sound-block decoder (sndunpack.c).
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechU8 g_soundUpsampleBuffer[0x400];
	extern MechU8 g_soundFrame[0x401];
	extern MechU8 g_soundDeltas[0x43];

	MechU8* DecodeSoundFrames(MechU8* p_src, MechU8* p_dst, MechU32 p_count, MechU32 p_frameSize, MechS32* p_state);

#ifdef __cplusplus
}
#endif

#endif // SNDUNPACK_H
