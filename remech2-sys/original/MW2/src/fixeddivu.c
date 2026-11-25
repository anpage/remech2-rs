/* In the original, FixedDivU16 is a C function whose body is an __asm block, like FixedDiv16. This
   is portable C in its place. */
#include "fixeddivu.h"

#include "portable.h"
#include "types.h"

// Divides two 16.16 fixed-point values like FixedDiv16, but with an unsigned divide.
int32_t FixedDivU16(int32_t p_a, int32_t p_b)
{
	/* The dividend is p_a sign-extended, then divided unsigned: the div faults for any negative
	   p_a but for a p_b above the dividend's high word. */
	return PortableS32(PortableDiv((uint64_t) ((int64_t) p_a * 0x10000), (uint32_t) p_b));
}
