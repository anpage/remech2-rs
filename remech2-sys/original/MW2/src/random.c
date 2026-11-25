#include "random.h"

#include "mechrand.h"
#include "types.h"

#include <math.h>

// Two tables of 127 random numbers, read round-robin: rand() values, and values roughly normally
// distributed around 0 (the sum of 30 rand() calls, scaled to +-sqrt(90) * 1024). rand is
// MechRand, with Visual C++'s 15-bit range: glibc's 31-bit numbers overflowed the sum.

int32_t g_normalRandomIndex = 0;

// A second pair of indices, so a second stream of callers doesn't disturb the first.

int32_t g_randomIndex2 = 0;

int32_t g_normalRandomIndex2 = 0;

int32_t g_normalRandomInts[127];

int32_t g_randomInts[127];

void InitRandom(uint32_t p_seed)
{
	int32_t sum;
	double scale;
	int32_t i;
	int32_t j;
	double range;

	MechSRand(p_seed);
	for (i = 0; i < 127; i++) {
		g_randomInts[i] = MechRand();
	}

	scale = (range = sqrt(90.0)) * 2.0 / (30 * MECH_RAND_MAX);
	for (i = 0; i < 127; i++) {
		sum = 0;
		for (j = 0; j < 30; j++) {
			sum += MechRand();
		}

		g_normalRandomInts[i] = (int32_t) ((sum * scale - range) * 1024.0);
	}
}

// RandomIntBelow is implemented on the Rust side. The original read g_randomInts round-robin
// through g_randomIndex, which at some framerates never gave a number below 3 when taken modulo a
// fixed delta time.

int32_t RandomNormal(void)
{
	int32_t value;

	value = g_normalRandomInts[g_normalRandomIndex];
	g_normalRandomIndex++;
	g_normalRandomIndex %= 127;
	return value;
}

int32_t RandomIntBelow2(int32_t p_max)
{
	int32_t value;

	value = g_randomInts[g_randomIndex2] % p_max;
	g_randomIndex2++;
	g_randomIndex2 %= 127;
	return value;
}

int32_t RandomNormal2(void)
{
	int32_t value;

	value = g_normalRandomInts[g_normalRandomIndex2];
	g_normalRandomIndex2++;
	g_normalRandomIndex2 %= 127;
	return value;
}

int32_t RandomNormalMean(void)
{
	return 0;
}

int32_t RandomNormalDeviation(void)
{
	return 0x400;
}
