/* In the original, ApproximateVectorLength is a C function whose body is an __asm block. This is
   portable C in its place. */
#include "approxlen.h"

#include "portable.h"
#include "types.h"

// Approximates the length of (p_x, p_y, p_z) as (max * 4 + the other two) / 4 of the absolute
// values.
// FUNCTION: MW2 0x100035c0
MechS32 ApproximateVectorLength(MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	/* neg leaves INT_MIN negative, and the comparisons are signed; the sum wraps. */
	MechS32 x = p_x < 0 ? PortableS32(0 - (MechU32) p_x) : p_x;
	MechS32 y = p_y < 0 ? PortableS32(0 - (MechU32) p_y) : p_y;
	MechS32 z = p_z < 0 ? PortableS32(0 - (MechU32) p_z) : p_z;
	MechS32 swap;

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

	return PortableSar32(PortableS32(((MechU32) x << 2) + (MechU32) y + (MechU32) z), 2);
}
