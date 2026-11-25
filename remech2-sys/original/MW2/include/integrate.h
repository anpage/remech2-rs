#ifndef INTEGRATE_H
#define INTEGRATE_H

#include "types.h"

// The functions and globals of integrate.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void IntegrateMidpoint(int32_t* p_position, int32_t* p_velocity, int32_t p_acceleration, int32_t p_time);

#ifdef __cplusplus
}
#endif

#endif // INTEGRATE_H
