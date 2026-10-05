#ifndef SHIFTDIV_H
#define SHIFTDIV_H

#include "types.h"

// The functions and globals of shiftdiv.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 ProjectCoordinate(MechS32 p_value, MechS32 p_depth, MechS32 p_shift, MechS32 p_center);

#ifdef __cplusplus
}
#endif

#endif // SHIFTDIV_H
