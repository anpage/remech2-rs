/* In the original, IsWithinRadius is a C function whose body is an __asm block. This is portable C
   in its place. */
#include "inradius.h"

#include "portable.h"
#include "types.h"

// Returns 1 if p_x^2 + p_y^2 + p_z^2 (64-bit) is at most p_radius^2.
// FUNCTION: MW2 0x10004ec0
MechS32 IsWithinRadius(MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_radius)
{
	/* The squares are compared unsigned, and a sum at or above the radius's square counts as
	   inside when their difference's low dword is 0. */
	MechU64 sum = (MechU64) ((MechS64) p_x * p_x) + (MechU64) ((MechS64) p_y * p_y) + (MechU64) ((MechS64) p_z * p_z);
	MechU64 square = (MechU64) ((MechS64) p_radius * p_radius);

	return sum < square || (MechU32) (sum - square) == 0;
}
