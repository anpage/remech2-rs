/* In the original, FixedSqrtGuess is a C function whose body is an __asm block. This is portable C
   in its place. */
#include "sqrtguess.h"

#include "portable.h"
#include "types.h"

// Estimates the square root of a 16.16 fixed-point value from its highest set bit, for
// FixedSqrt16 to refine.
uint32_t FixedSqrtGuess(uint32_t p_value)
{
	/* bsr of 0 leaves the shift undefined */
	int32_t bit;

	PORTABLE_ASSERT(p_value != 0);
	bit = PortableBsr(p_value);

	if (bit > 15) {
		return p_value >> ((bit - 15) >> 1);
	}

	return p_value << ((16 - bit) >> 1);
}
