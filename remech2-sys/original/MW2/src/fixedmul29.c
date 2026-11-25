/* In the original, FixedMul29 is a C function whose body is an __asm block. This is portable C in
   its place. */
#include "fixedmul29.h"

#include "portable.h"
#include "types.h"

// p_a * p_b >> 29, rounded.
int32_t FixedMul29(int32_t p_a, int32_t p_b)
{
	return PortableS32(PortableShrdRound((uint64_t) ((int64_t) p_a * p_b), 29));
}
