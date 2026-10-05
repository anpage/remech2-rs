/* In the original, ProjectRadius is a C function whose body is an __asm block. This is portable C
   in its place. */
#include "muldiv14.h"

#include "portable.h"
#include "types.h"

// Returns p_scale * p_radius / p_depth / 2^14, the product kept in 64 bits and shifted by 8 before the division
// and the quotient by 6 after it.
// FUNCTION: MW2 0x10013340
MechS32 ProjectRadius(MechS32 p_scale, MechS32 p_radius, MechS32 p_depth)
{
	MechS64 scaled = PortableSar64((MechS64) p_scale * p_radius, 8);

	return PortableSar32(PortableIdiv(scaled, p_depth), 6);
}
