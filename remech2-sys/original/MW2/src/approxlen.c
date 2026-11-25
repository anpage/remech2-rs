/* In the original, ApproximateVectorLength is a C function whose body is an __asm block. This is
   portable C in its place. */
#include "approxlen.h"

#include "portable.h"
#include "types.h"

// Approximates the length of (p_x, p_y, p_z) as (max * 4 + the other two) / 4 of the absolute
// values.
int32_t ApproximateVectorLength(int32_t p_x, int32_t p_y, int32_t p_z)
{
	/* neg leaves INT_MIN negative, and the comparisons are signed; the sum wraps. */
	int32_t x = p_x < 0 ? PortableS32(0 - (uint32_t) p_x) : p_x;
	int32_t y = p_y < 0 ? PortableS32(0 - (uint32_t) p_y) : p_y;
	int32_t z = p_z < 0 ? PortableS32(0 - (uint32_t) p_z) : p_z;
	int32_t swap;

	if (x < y) {
		swap = x;
		x = y;
		y = swap;
	}

	if (x < z) {
		swap = x;
		x = z;
		z = swap;
	}

	return PortableSar32(PortableS32(((uint32_t) x << 2) + (uint32_t) y + (uint32_t) z), 2);
}
