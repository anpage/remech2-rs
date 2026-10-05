/* In the original, FixedMul16 is a C function whose body is an __asm block. This is portable C in
   its place. */
#include "fixedmul.h"

#include "portable.h"
#include "types.h"

// FUNCTION: MW2 0x10003580
MechS32 FixedMul16(MechS32 p_a, MechS32 p_b)
{
	return PortableS32(PortableShrdRound((MechU64) ((MechS64) p_a * p_b), 16));
}
