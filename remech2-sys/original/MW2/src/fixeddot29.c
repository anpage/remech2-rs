/* In the original, FixedDot29 is a C function whose body is an __asm block. This is portable C in
   its place. */
#include "fixeddot29.h"

#include "portable.h"
#include "types.h"

// Returns p_a * p_b + p_c * p_d + p_e * p_f, shifted right by 29 and rounded.
// FUNCTION: MW2 0x10042700
MechS32 FixedDot29(MechS32 p_a, MechS32 p_b, MechS32 p_c, MechS32 p_d, MechS32 p_e, MechS32 p_f)
{
	/* The sum wraps at 64 bits, like the add/adc chain. */
	MechU64 sum = (MechU64) ((MechS64) p_a * p_b) + (MechU64) ((MechS64) p_c * p_d) + (MechU64) ((MechS64) p_e * p_f);

	return PortableS32(PortableShrdRound(sum, 29));
}
