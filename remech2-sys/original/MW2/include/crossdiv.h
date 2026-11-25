#ifndef CROSSDIV_H
#define CROSSDIV_H

#include "types.h"

// The functions and globals of crossdiv.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t CrossDiv(int32_t p_a, int32_t p_b, int32_t p_c, int32_t p_d, int32_t p_divisor);

#ifdef __cplusplus
}
#endif

#endif // CROSSDIV_H
