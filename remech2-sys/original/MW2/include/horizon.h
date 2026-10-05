#ifndef HORIZON_H
#define HORIZON_H

#include "types.h"

struct Eyepoint;

// The functions of horizon.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 IsAboveHorizon(MechS32 p_x, MechS32 p_y, struct Eyepoint* p_eyepoint);
	MechS32 HorizonYAtX(MechS32 p_x, struct Eyepoint* p_eyepoint);
	MechS32 HorizonXAtY(MechS32 p_y, struct Eyepoint* p_eyepoint);

#ifdef __cplusplus
}
#endif

#endif // HORIZON_H
