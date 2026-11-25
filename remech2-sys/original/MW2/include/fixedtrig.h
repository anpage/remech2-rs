#ifndef FIXEDTRIG_H
#define FIXEDTRIG_H

#include "types.h"

// The functions and globals of fixedtrig.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t FixedSin(int32_t p_angle);
	int32_t FixedCos(int32_t p_angle);
	int32_t FixedAsin(int32_t p_sine);
	int32_t FixedAcos(int32_t p_cosine);
	int32_t FixedAtan2(int32_t p_x, int32_t p_z);

#ifdef __cplusplus
}
#endif

#endif // FIXEDTRIG_H
