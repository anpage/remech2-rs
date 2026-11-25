/* In the original, FixedMul16 is a C function whose body is an __asm block. This is portable C in
   its place. */
#include "fixedmul.h"

#include "portable.h"
#include "types.h"

int32_t FixedMul16(int32_t p_a, int32_t p_b)
{
	return PortableS32(PortableShrdRound((MechU64) ((MechS64) p_a * p_b), 16));
}
