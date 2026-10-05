#ifndef RANDOM_H
#define RANDOM_H

#include "types.h"

// The functions and globals of random.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_normalRandomIndex;
	extern MechS32 g_randomIndex2;
	extern MechS32 g_normalRandomIndex2;
	extern MechS32 g_normalRandomInts[127];
	extern MechS32 g_randomInts[127];

	void InitRandom(MechU32 p_seed);
	// Implemented on the Rust side (src/sim/math.rs)
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
