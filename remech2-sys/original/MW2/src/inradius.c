/* In the original, IsWithinRadius is a C function whose body is an __asm block. This is portable C
   in its place. */
#include "inradius.h"

#include "portable.h"
#include "types.h"

// Returns 1 if p_x^2 + p_y^2 + p_z^2 (64-bit) is at most p_radius^2.
int32_t IsWithinRadius(int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_radius)
{
	/* The squares are compared unsigned, and a sum at or above the radius's square counts as
	   inside when their difference's low dword is 0. */
	uint64_t sum =
		(uint64_t) ((int64_t) p_x * p_x) + (uint64_t) ((int64_t) p_y * p_y) + (uint64_t) ((int64_t) p_z * p_z);
	uint64_t square = (uint64_t) ((int64_t) p_radius * p_radius);

	return sum < square || (uint32_t) (sum - square) == 0;
}
