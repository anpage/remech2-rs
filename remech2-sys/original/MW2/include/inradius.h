#ifndef INRADIUS_H
#define INRADIUS_H

#include "types.h"

// The functions and globals of inradius.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t IsWithinRadius(int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_radius);

#ifdef __cplusplus
}
#endif

#endif // INRADIUS_H
