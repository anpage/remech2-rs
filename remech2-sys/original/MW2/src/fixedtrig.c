/* In the original, FixedSin (sine), FixedAsin (arcsine) and FixedAtan2 (arctangent) are C functions
   with __asm bodies over clock.c's tables. This is portable C in their place. */
#include "fixedtrig.h"

#include "clock.h"
#include "decomp.h"
#include "portable.h"
#include "types.h"

// Returns the sine of p_angle (16.16 degrees) from the table, as 16.16: the angle is scaled
// to 1024 steps to the circle (0x5b05b05b is 2^37 / 360), and the table's quarter wave is
// interpolated, mirrored and negated by quadrant.
int32_t FixedSin(int32_t p_angle)
{
	int32_t result;

	/* The angle in 1024ths of the circle (8.16 per quadrant): bit 24 selects the falling half
	   of a half wave, bit 25 the negative half wave. */
	uint32_t steps = PortableShrdRound((MechU64) ((MechS64) p_angle * 0x5b05b05b), 29);
	uint32_t index = (steps >> 16) & 0xff;
	uint32_t fraction = steps;
	uint32_t low;
	uint32_t sine;

	if (steps & 0x1000000) {
		fraction = ~steps;
		index ^= 0xff;
	}

	low = (uint32_t) g_sinTable[index];
	sine = low + PortableShrdRound((MechU64) ((uint32_t) g_sinTable[index + 1] - low) * (fraction & 0xffff), 16);
	if (steps & 0x2000000) {
		sine = 0 - sine;
	}

	result = PortableS32(sine);

	return result;
}

// Returns the cosine of p_angle (16.16 degrees): the sine 90 degrees on.
int32_t FixedCos(int32_t p_angle)
{
	/* The add wraps (BuildMatrixEx's portable C calls this with any angle). */
	return FixedSin(PortableS32((uint32_t) p_angle + 0x5a0000));
}

// Returns the arcsine of p_sine (2.29 fixed point, as the table holds it) in 16.16 degrees: a
// binary search of the table's quarter wave, interpolated between the two entries found.
int32_t FixedAsin(int32_t p_sine)
{
	/* The magnitude wraps like the C's negation (INT_MIN stays negative), and the search
	   compares unsigned. */
	uint32_t sine = (uint32_t) p_sine;
	int32_t negative = 0;
	int32_t result;
	uint32_t index;
	uint32_t step;
	uint32_t position;
	uint32_t offset;
	uint32_t span;

	if (p_sine == 0) {
		return 0;
	}

	if (p_sine < 0) {
		negative++;
		sine = 0 - sine;
	}

	if (PortableS32(sine) >= 0x20000000) {
		result = 0x5a0000;
	}
	else {
		index = 0;
		for (step = 0x80; step; step >>= 1) {
			if (sine >= (uint32_t) g_sinTable[index + step]) {
				index += step;
			}
		}

		/* The entry in 8.16 steps, plus the fraction between it and the next one (the idiv faults
		   on two equal entries, which the game's table doesn't have). */
		position = index << 16;
		offset = sine - (uint32_t) g_sinTable[index];
		if (offset) {
			span = (uint32_t) g_sinTable[index + 1] - (uint32_t) g_sinTable[index];
			if (span && offset >= span) {
				position += 0x10000;
			}
			else {
				position =
					(position & 0xffff0000) |
					((uint32_t) PortableIdiv((MechS64) PortableS32(offset) * 0x10000, PortableS32(span)) & 0xffff);
			}
		}

		result = (int32_t) (((MechS64) 0x5a000000 * PortableS32(position)) >> 32);
	}

	return negative ? -result : result;
}

int32_t FixedAcos(int32_t p_cosine)
{
	return 0x5a0000 - FixedAsin(p_cosine);
}

// Returns the bearing of (p_x, p_z) in 16.16 degrees: the arctangent of the smaller over the
// larger component from the table, folded into its octant. The result is also left in dx:ax.
int32_t FixedAtan2(int32_t p_x, int32_t p_z)
{
	/* The magnitudes wrap like neg (INT_MIN stays negative) and compare signed; the quotient,
	   interpolation and folding are unsigned. */
	int32_t smaller = p_x < 0 ? PortableS32(0 - (uint32_t) p_x) : p_x;
	int32_t larger = p_z < 0 ? PortableS32(0 - (uint32_t) p_z) : p_z;
	uint32_t octant = (p_x < 0 ? 1 : 0) | (p_z < 0 ? 2 : 0);
	uint32_t angle;
	uint32_t ratio;
	uint32_t index;
	uint32_t product;
	int32_t swap;

	if (smaller == larger) {
		angle = 0x2d0000;
	}
	else {
		if (smaller > larger) {
			swap = smaller;
			smaller = larger;
			larger = swap;
			octant |= 4;
		}

		if (smaller == 0) {
			angle = 0;
		}
		else {
			/* The ratio in 8.24 fixed point */
			ratio = PortableDiv((MechU64) (uint32_t) smaller << 24, (uint32_t) larger);
			index = (ratio >> 16) & 0xff;
			product = (ratio & 0xffff) * ((uint32_t) g_atanTable[index + 1] - (uint32_t) g_atanTable[index]);
			angle = (product >> 16) + (uint32_t) g_atanTable[index] + ((product >> 15) & 1);
		}
	}

	if (octant & 4) {
		angle = 0x5a0000 - angle;
	}

	if (octant & 2) {
		angle = 0xb40000 - angle;
	}

	if (octant & 1) {
		angle = 0 - angle;
	}

	return PortableS32(angle);
}
