/* In the original, CrossDiv is a C function whose body is an __asm block, like MulDiv64. This is
   portable C in its place. */
#include "crossdiv.h"

#include "portable.h"
#include "types.h"

// Divides the 64-bit p_a * p_d - p_b * p_c by p_divisor.
// FUNCTION: MW2 0x100349c0
MechS32 CrossDiv(MechS32 p_a, MechS32 p_b, MechS32 p_c, MechS32 p_d, MechS32 p_divisor)
{
	/* The difference of the products fits in 64 bits. */
	return PortableIdiv((MechS64) p_a * p_d - (MechS64) p_b * p_c, p_divisor);
}
