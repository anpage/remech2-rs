#ifndef HORIZON_H
#define HORIZON_H

#include "types.h"

struct Eyepoint;

// The functions of horizon.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t IsAboveHorizon(int32_t p_x, int32_t p_y, struct Eyepoint* p_eyepoint);
	int32_t HorizonYAtX(int32_t p_x, struct Eyepoint* p_eyepoint);
	int32_t HorizonXAtY(int32_t p_y, struct Eyepoint* p_eyepoint);

#ifdef __cplusplus
}
#endif

#endif // HORIZON_H
