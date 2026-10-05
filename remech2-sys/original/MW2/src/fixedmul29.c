/* In the original, FixedMul29 is a C function whose body is an __asm block. This is portable C in
   its place. */
#include "fixedmul29.h"

#include "portable.h"
#include "types.h"

// p_a * p_b >> 29, rounded.
// FUNCTION: MW2 0x10019ad0
MechS32 FixedMul29(MechS32 p_a, MechS32 p_b)
{
	return PortableS32(PortableShrdRound((MechU64) ((MechS64) p_a * p_b), 29));
}
