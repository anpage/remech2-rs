/* In the original, UnprojectCoordinate is a C function whose body is an __asm block, like MulDiv64.
   This is portable C in its place. */
#include "scaledelta.h"

#include "portable.h"
#include "types.h"

// Scales the difference p_screen - p_center by 4 * p_depth, as a 64-bit product shifted right by p_shift.
// FUNCTION: MW2 0x10034990
MechS32 UnprojectCoordinate(MechS32 p_screen, MechS32 p_center, MechS32 p_depth, MechS32 p_shift)
{
	/* The difference and its scaling wrap at 32 bits; shrd takes the count modulo 32. */
	MechS32 difference = PortableS32(((MechU32) p_screen - (MechU32) p_center) << 2);

	return PortableS32((MechU32) ((MechU64) ((MechS64) difference * p_depth) >> (p_shift & 31)));
}
