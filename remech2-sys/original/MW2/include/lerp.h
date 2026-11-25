#ifndef LERP_H
#define LERP_H

#include "types.h"

// The functions and globals of lerp.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t Lerp(int32_t p_x0, int32_t p_x1, int32_t p_x, int32_t p_y0, int32_t p_y1);

#ifdef __cplusplus
}
#endif

#endif // LERP_H
