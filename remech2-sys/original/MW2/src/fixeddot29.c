/* In the original, FixedDot29 is a C function whose body is an __asm block. This is portable C in
   its place. */
#include "fixeddot29.h"

#include "portable.h"
#include "types.h"

// Returns p_a * p_b + p_c * p_d + p_e * p_f, shifted right by 29 and rounded.
int32_t FixedDot29(int32_t p_a, int32_t p_b, int32_t p_c, int32_t p_d, int32_t p_e, int32_t p_f)
{
	/* The sum wraps at 64 bits, like the add/adc chain. */
	uint64_t sum =
		(uint64_t) ((int64_t) p_a * p_b) + (uint64_t) ((int64_t) p_c * p_d) + (uint64_t) ((int64_t) p_e * p_f);

	return PortableS32(PortableShrdRound(sum, 29));
}
