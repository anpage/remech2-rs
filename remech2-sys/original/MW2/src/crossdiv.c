/* In the original, CrossDiv is a C function whose body is an __asm block, like MulDiv64. This is
   portable C in its place. */
#include "crossdiv.h"

#include "portable.h"
#include "types.h"

// Divides the 64-bit p_a * p_d - p_b * p_c by p_divisor.
int32_t CrossDiv(int32_t p_a, int32_t p_b, int32_t p_c, int32_t p_d, int32_t p_divisor)
{
	/* The difference of the products fits in 64 bits. */
	return PortableIdiv((MechS64) p_a * p_d - (MechS64) p_b * p_c, p_divisor);
}
