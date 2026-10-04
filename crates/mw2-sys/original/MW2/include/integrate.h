#ifndef INTEGRATE_H
#define INTEGRATE_H

#include "types.h"

// The functions and globals of integrate.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void IntegrateMidpoint(MechS32* p_position, MechS32* p_velocity, MechS32 p_acceleration, MechS32 p_time);

#ifdef __cplusplus
}
#endif

#endif // INTEGRATE_H
