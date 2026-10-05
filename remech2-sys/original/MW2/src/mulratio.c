/* In the original, MulRatio is a C function whose body is an __asm block. This is portable C in its
   place. */
#include "mulratio.h"

#include "portable.h"
#include "types.h"

// p_a * p_b / (p_b + p_c) >> 12.
// FUNCTION: MW2 0x1004c800
MechS32 MulRatio(MechS32 p_a, MechS32 p_b, MechS32 p_c)
{
	/* The divisor wraps at 32 bits, and the quotient is shifted unsigned. */
	MechS32 divisor = PortableS32((MechU32) p_b + (MechU32) p_c);

	return (MechS32) ((MechU32) PortableIdiv((MechS64) p_a * p_b, divisor) >> 12);
}
