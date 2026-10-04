#ifndef SNDUNPACK_H
#define SNDUNPACK_H

#include "types.h"

// The sound-block decoder of sndunpack.asm (sndunpack.c in COMPAT_MODE).
#ifdef __cplusplus
extern "C"
{
#endif

	MechU8* DecodeSoundFrames(MechU8* p_src, MechU8* p_dst, MechU32 p_count, MechU32 p_frameSize, MechS32* p_state);

#ifdef __cplusplus
}
#endif

// sndunpack.asm's routines and data. reccmp reads annotations from C sources only, so they're
// here, by name.

// FUNCTION: MW2 0x1001a63c
// DecodeSoundFrames

// FUNCTION: MW2 0x1001a87b
// UpsampleSoundFrame4

// FUNCTION: MW2 0x1001a8d3
// UpsampleSoundFrame2

// GLOBAL: MW2 0x100a2f04
// g_soundUpsampleBuffer

// GLOBAL: MW2 0x100a3304
// g_soundFrame

// GLOBAL: MW2 0x100a3705
// g_soundDeltas

#endif // SNDUNPACK_H
