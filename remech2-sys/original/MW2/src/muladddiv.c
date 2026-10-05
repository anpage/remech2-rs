/* In the original, MulAddDiv is a C function whose body is an __asm block. This is portable C in
   its place. */
#include "muladddiv.h"

#include "portable.h"
#include "types.h"

// Returns (p_a * p_b + (p_c << 16)) / p_d, with a 64-bit intermediate.
// FUNCTION: MW2 0x1004c860
MechS32 MulAddDiv(MechS32 p_a, MechS32 p_b, MechS32 p_c, MechS32 p_d)
{
	return PortableIdiv((MechS64) p_a * p_b + (MechS64) p_c * 0x10000, p_d);
}
