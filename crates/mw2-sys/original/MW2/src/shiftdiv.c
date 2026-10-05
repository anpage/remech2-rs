/* In the original, ProjectCoordinate is a C function whose body is an __asm block. This is portable
   C in its place. */
#include "shiftdiv.h"

#include "portable.h"
#include "types.h"

// Returns (p_value << p_shift) / p_depth, divided by 4 and rounded, plus p_center.
// FUNCTION: MW2 0x10042740
MechS32 ProjectCoordinate(MechS32 p_value, MechS32 p_depth, MechS32 p_shift, MechS32 p_center)
{
	/* shld/shl take the count modulo 32; the rounding and the sum wrap. */
	MechU32 quotient = (MechU32) PortableIdiv((MechS64) p_value * ((MechS64) 1 << (p_shift & 31)), p_depth);

	return PortableS32((MechU32) PortableSar32(PortableS32(quotient + 2), 2) + (MechU32) p_center);
}
