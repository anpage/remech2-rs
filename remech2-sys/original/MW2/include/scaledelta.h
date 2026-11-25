#ifndef SCALEDELTA_H
#define SCALEDELTA_H

#include "types.h"

// The functions and globals of scaledelta.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t UnprojectCoordinate(int32_t p_screen, int32_t p_center, int32_t p_depth, int32_t p_shift);

#ifdef __cplusplus
}
#endif

#endif // SCALEDELTA_H
