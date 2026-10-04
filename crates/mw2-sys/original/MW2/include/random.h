#ifndef RANDOM_H
#define RANDOM_H

#include "types.h"

// The functions and globals of random.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void InitRandom(MechU32 p_seed);
	MechS32 RandomIntBelow(MechS32 p_max);
	MechS32 RandomNormal(void);
	MechS32 RandomIntBelow2(MechS32 p_max);
	MechS32 RandomNormal2(void);
	MechS32 RandomNormalMean(void);
	MechS32 RandomNormalDeviation(void);

#ifdef __cplusplus
}
#endif

#endif // RANDOM_H
