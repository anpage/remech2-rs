#ifndef RANDOM_H
#define RANDOM_H

#include "types.h"

// The functions and globals of random.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_normalRandomIndex;
	extern int32_t g_randomIndex2;
	extern int32_t g_normalRandomIndex2;
	extern int32_t g_normalRandomInts[127];
	extern int32_t g_randomInts[127];

	void InitRandom(uint32_t p_seed);
	// Implemented on the Rust side (src/sim/math.rs)
	int32_t RandomIntBelow(int32_t p_max);
	int32_t RandomNormal(void);
	int32_t RandomIntBelow2(int32_t p_max);
	int32_t RandomNormal2(void);
	int32_t RandomNormalMean(void);
	int32_t RandomNormalDeviation(void);

#ifdef __cplusplus
}
#endif

#endif // RANDOM_H
