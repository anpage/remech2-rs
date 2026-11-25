/* In the original, UnprojectCoordinate is a C function whose body is an __asm block, like MulDiv64.
   This is portable C in its place. */
#include "scaledelta.h"

#include "portable.h"
#include "types.h"

// Scales the difference p_screen - p_center by 4 * p_depth, as a 64-bit product shifted right by p_shift.
int32_t UnprojectCoordinate(int32_t p_screen, int32_t p_center, int32_t p_depth, int32_t p_shift)
{
	/* The difference and its scaling wrap at 32 bits; shrd takes the count modulo 32. */
	int32_t difference = PortableS32(((uint32_t) p_screen - (uint32_t) p_center) << 2);

	return PortableS32((uint32_t) ((uint64_t) ((int64_t) difference * p_depth) >> (p_shift & 31)));
}
