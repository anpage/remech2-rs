/* In the original, FixedDivU16 is a C function whose body is an __asm block, like FixedDiv16. This
   is portable C in its place. */
#include "fixeddivu.h"

#include "portable.h"
#include "types.h"

// Divides two 16.16 fixed-point values like FixedDiv16, but with an unsigned divide.
// FUNCTION: MW2 0x10044700
MechS32 FixedDivU16(MechS32 p_a, MechS32 p_b)
{
	/* The dividend is p_a sign-extended, then divided unsigned: the div faults for any negative
	   p_a but for a p_b above the dividend's high word. */
	return PortableS32(PortableDiv((MechU64) ((MechS64) p_a * 0x10000), (MechU32) p_b));
}
