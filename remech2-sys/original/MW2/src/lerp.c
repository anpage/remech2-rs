/* In the original, Lerp is a C function whose body is an __asm block, like MulDiv64. This is
   portable C in its place. */
#include "lerp.h"

#include "portable.h"
#include "types.h"

// Interpolates linearly: the y at p_x on the line through (p_x0, p_y0) and (p_x1, p_y1), measured
// from whichever end has the larger x.
int32_t Lerp(int32_t p_x0, int32_t p_x1, int32_t p_x, int32_t p_y0, int32_t p_y1)
{
	/* The differences and the sum wrap at 32 bits. */
	int32_t run;
	int64_t rise;
	int32_t base;

	if (p_x1 > p_x0) {
		run = PortableS32((uint32_t) p_x0 - (uint32_t) p_x1);
		rise = (int64_t) PortableS32((uint32_t) p_x - (uint32_t) p_x1) * PortableS32((uint32_t) p_y0 - (uint32_t) p_y1);
		base = p_y1;
	}
	else {
		run = PortableS32((uint32_t) p_x1 - (uint32_t) p_x0);
		rise = (int64_t) PortableS32((uint32_t) p_x - (uint32_t) p_x0) * PortableS32((uint32_t) p_y1 - (uint32_t) p_y0);
		base = p_y0;
	}

	return PortableS32((uint32_t) PortableIdiv(rise, run) + (uint32_t) base);
}
