/* In the original, FixedDiv29 is a C function whose body is an __asm block, like FixedDiv16. This
   is portable C in its place. */
#include "fixeddiv29.h"

#include "portable.h"
#include "types.h"

// Divides p_a, shifted left by 29, by p_b.
MechS32 FixedDiv29(MechS32 p_a, MechS32 p_b)
{
	return PortableIdiv((MechS64) p_a * 0x20000000, p_b);
}
