/* In the original, FixedMul30 is a C function whose body is an __asm block, like FixedMul16. This
   is portable C in its place. */
#include "fixedmul30.h"

#include "portable.h"
#include "types.h"

// Multiplies two 2.30 fixed-point values, rounding.
int32_t FixedMul30(int32_t p_a, int32_t p_b)
{
	return PortableS32(PortableShrdRound((uint64_t) ((int64_t) p_a * p_b), 30));
}
