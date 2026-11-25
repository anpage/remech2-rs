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
	MechU64 sum = (MechU64) ((MechS64) p_x * p_x) + (MechU64) ((MechS64) p_y * p_y) + (MechU64) ((MechS64) p_z * p_z);
	MechU64 square = (MechU64) ((MechS64) p_radius * p_radius);

	return sum < square || (uint32_t) (sum - square) == 0;
}
