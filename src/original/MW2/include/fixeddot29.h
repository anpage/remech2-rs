#ifndef FIXEDDOT29_H
#define FIXEDDOT29_H

#include "types.h"

// The functions and globals of fixeddot29.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 FixedDot29(MechS32 p_a, MechS32 p_b, MechS32 p_c, MechS32 p_d, MechS32 p_e, MechS32 p_f);

#ifdef __cplusplus
}
#endif

#endif // FIXEDDOT29_H
