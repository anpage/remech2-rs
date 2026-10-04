#ifndef INRADIUS_H
#define INRADIUS_H

#include "types.h"

// The functions and globals of inradius.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 IsWithinRadius(MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_radius);

#ifdef __cplusplus
}
#endif

#endif // INRADIUS_H
