#ifndef MULDIV_H
#define MULDIV_H

#include "types.h"

// The functions and globals of muldiv.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 MulDiv64(MechS32 p_a, MechS32 p_b, MechS32 p_c);

#ifdef __cplusplus
}
#endif

#endif // MULDIV_H
