#include "random.h"

#include "types.h"

#include <math.h>
#include <stdlib.h>

// Two tables of 127 random numbers, read round-robin: rand() values, and values roughly normally
// distributed around 0 (the sum of 30 rand() calls, scaled to +-sqrt(90) * 1024).

// GLOBAL: MW2 0x100ae750
MechS32 g_normalRandomIndex = 0;

// A second pair of indices, so a second stream of callers doesn't disturb the first.

// GLOBAL: MW2 0x100ae754
MechS32 g_randomIndex2 = 0;

// GLOBAL: MW2 0x100ae758
MechS32 g_normalRandomIndex2 = 0;

// GLOBAL: MW2 0x100c2d20
MechS32 g_normalRandomInts[127];

// GLOBAL: MW2 0x100c2f20
MechS32 g_randomInts[127];

// Stack-slot permutation: sum, scale, i, j and range.
// FUNCTION: MW2 0x100735e0
void InitRandom(MechU32 p_seed)
{
	MechS32 sum;
	MechDouble scale;
	MechS32 i;
	MechS32 j;
	MechDouble range;

	srand(p_seed);
	for (i = 0; i < 127; i++) {
		g_randomInts[i] = rand();
	}

	scale = (range = sqrt(90.0)) * 2.0 / (30 * RAND_MAX);
	for (i = 0; i < 127; i++) {
		sum = 0;
		for (j = 0; j < 30; j++) {
			sum += rand();
		}

		g_normalRandomInts[i] = (MechS32) ((sum * scale - range) * 1024.0);
	}
}

// RandomIntBelow is implemented on the Rust side. The original read g_randomInts round-robin
// through g_randomIndex (0x100ae74c), which at some framerates never gave a number below 3 when
// taken modulo a fixed delta time.

// FUNCTION: MW2 0x100736f5
MechS32 RandomNormal(void)
{
	MechS32 value;

	value = g_normalRandomInts[g_normalRandomIndex];
	g_normalRandomIndex++;
	g_normalRandomIndex %= 127;
	return value;
}

// FUNCTION: MW2 0x10073733
MechS32 RandomIntBelow2(MechS32 p_max)
{
	MechS32 value;

	value = g_randomInts[g_randomIndex2] % p_max;
	g_randomIndex2++;
	g_randomIndex2 %= 127;
	return value;
}

// FUNCTION: MW2 0x10073775
MechS32 RandomNormal2(void)
{
	MechS32 value;

	value = g_normalRandomInts[g_normalRandomIndex2];
	g_normalRandomIndex2++;
	g_normalRandomIndex2 %= 127;
	return value;
}

// FUNCTION: MW2 0x100737b3
MechS32 RandomNormalMean(void)
{
	return 0;
}

// FUNCTION: MW2 0x100737c5
MechS32 RandomNormalDeviation(void)
{
	return 0x400;
}
