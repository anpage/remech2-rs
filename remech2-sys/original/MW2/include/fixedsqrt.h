#ifndef FIXEDSQRT_H
#define FIXEDSQRT_H

#include "types.h"

// The functions and globals of fixedsqrt.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	uint32_t FixedSqrt16(uint32_t p_value);
	void NormalizeVectorGuarded(int32_t* p_x, int32_t* p_y, int32_t* p_z);

#ifdef __cplusplus
}
#endif

#endif // FIXEDSQRT_H
