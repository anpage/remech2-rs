#ifndef APPROXLEN_H
#define APPROXLEN_H

#include "types.h"

// The functions and globals of approxlen.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t ApproximateVectorLength(int32_t p_x, int32_t p_y, int32_t p_z);

#ifdef __cplusplus
}
#endif

#endif // APPROXLEN_H
