#ifndef SCALEDELTA_H
#define SCALEDELTA_H

#include "types.h"

// The functions and globals of scaledelta.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 UnprojectCoordinate(MechS32 p_screen, MechS32 p_center, MechS32 p_depth, MechS32 p_shift);

#ifdef __cplusplus
}
#endif

#endif // SCALEDELTA_H
