#ifndef APPROXLEN_H
#define APPROXLEN_H

#include "types.h"

// The functions and globals of approxlen.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 ApproximateVectorLength(MechS32 p_x, MechS32 p_y, MechS32 p_z);

#ifdef __cplusplus
}
#endif

#endif // APPROXLEN_H
