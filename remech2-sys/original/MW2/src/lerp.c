/* In the original, Lerp is a C function whose body is an __asm block, like MulDiv64. This is
   portable C in its place. */
#include "lerp.h"

#include "portable.h"
#include "types.h"

// Interpolates linearly: the y at p_x on the line through (p_x0, p_y0) and (p_x1, p_y1), measured
// from whichever end has the larger x.
// FUNCTION: MW2 0x100349f0
MechS32 Lerp(MechS32 p_x0, MechS32 p_x1, MechS32 p_x, MechS32 p_y0, MechS32 p_y1)
{
	/* The differences and the sum wrap at 32 bits. */
	MechS32 run;
	MechS64 rise;
	MechS32 base;

	if (p_x1 > p_x0) {
		run = PortableS32((MechU32) p_x0 - (MechU32) p_x1);
		rise = (MechS64) PortableS32((MechU32) p_x - (MechU32) p_x1) * PortableS32((MechU32) p_y0 - (MechU32) p_y1);
		base = p_y1;
	}
	else {
		run = PortableS32((MechU32) p_x1 - (MechU32) p_x0);
		rise = (MechS64) PortableS32((MechU32) p_x - (MechU32) p_x0) * PortableS32((MechU32) p_y1 - (MechU32) p_y0);
		base = p_y0;
	}

	return PortableS32((MechU32) PortableIdiv(rise, run) + (MechU32) base);
}
