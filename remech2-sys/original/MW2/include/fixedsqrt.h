#ifndef FIXEDSQRT_H
#define FIXEDSQRT_H

#include "types.h"

// The functions and globals of fixedsqrt.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechU32 FixedSqrt16(MechU32 p_value);
	void NormalizeVectorGuarded(MechS32* p_x, MechS32* p_y, MechS32* p_z);

#ifdef __cplusplus
}
#endif

#endif // FIXEDSQRT_H
