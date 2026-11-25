#ifndef SHIFTDIV_H
#define SHIFTDIV_H

#include "types.h"

// The functions and globals of shiftdiv.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t ProjectCoordinate(int32_t p_value, int32_t p_depth, int32_t p_shift, int32_t p_center);

#ifdef __cplusplus
}
#endif

#endif // SHIFTDIV_H
