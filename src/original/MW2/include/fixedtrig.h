#ifndef FIXEDTRIG_H
#define FIXEDTRIG_H

#include "types.h"

// The functions and globals of fixedtrig.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 FixedSin(MechS32 p_angle);
	MechS32 FixedCos(MechS32 p_angle);
	MechS32 FixedAsin(MechS32 p_sine);
	MechS32 FixedAcos(MechS32 p_cosine);
	MechS32 FixedAtan2(MechS32 p_x, MechS32 p_z);

#ifdef __cplusplus
}
#endif

#endif // FIXEDTRIG_H
