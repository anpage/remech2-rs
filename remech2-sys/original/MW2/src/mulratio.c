/* In the original, MulRatio is a C function whose body is an __asm block. This is portable C in its
   place. */
#include "mulratio.h"

#include "portable.h"
#include "types.h"

// p_a * p_b / (p_b + p_c) >> 12.
int32_t MulRatio(int32_t p_a, int32_t p_b, int32_t p_c)
{
	/* The divisor wraps at 32 bits, and the quotient is shifted unsigned. */
	int32_t divisor = PortableS32((uint32_t) p_b + (uint32_t) p_c);

	return (int32_t) ((uint32_t) PortableIdiv((int64_t) p_a * p_b, divisor) >> 12);
}
