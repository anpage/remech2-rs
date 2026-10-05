/* In the original, FixedMul30 is a C function whose body is an __asm block, like FixedMul16. This
   is portable C in its place. */
#include "fixedmul30.h"

#include "portable.h"
#include "types.h"

// Multiplies two 2.30 fixed-point values, rounding.
// FUNCTION: MW2 0x10044720
MechS32 FixedMul30(MechS32 p_a, MechS32 p_b)
{
	return PortableS32(PortableShrdRound((MechU64) ((MechS64) p_a * p_b), 30));
}
