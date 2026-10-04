#ifndef LERP_H
#define LERP_H

#include "types.h"

// The functions and globals of lerp.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 Lerp(MechS32 p_x0, MechS32 p_x1, MechS32 p_x, MechS32 p_y0, MechS32 p_y1);

#ifdef __cplusplus
}
#endif

#endif // LERP_H
