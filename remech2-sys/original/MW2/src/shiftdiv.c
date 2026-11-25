/* In the original, ProjectCoordinate is a C function whose body is an __asm block. This is portable
   C in its place. */
#include "shiftdiv.h"

#include "portable.h"
#include "types.h"

// Returns (p_value << p_shift) / p_depth, divided by 4 and rounded, plus p_center.
int32_t ProjectCoordinate(int32_t p_value, int32_t p_depth, int32_t p_shift, int32_t p_center)
{
	/* shld/shl take the count modulo 32; the rounding and the sum wrap. */
	uint32_t quotient = (uint32_t) PortableIdiv((int64_t) p_value * ((int64_t) 1 << (p_shift & 31)), p_depth);

	return PortableS32((uint32_t) PortableSar32(PortableS32(quotient + 2), 2) + (uint32_t) p_center);
}
