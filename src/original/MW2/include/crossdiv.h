#ifndef CROSSDIV_H
#define CROSSDIV_H

#include "types.h"

// The functions and globals of crossdiv.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 CrossDiv(MechS32 p_a, MechS32 p_b, MechS32 p_c, MechS32 p_d, MechS32 p_divisor);

#ifdef __cplusplus
}
#endif

#endif // CROSSDIV_H
