/* In the original, FixedDiv29 is a C function whose body is an __asm block, like FixedDiv16. This
   is portable C in its place. */
#include "fixeddiv29.h"

#include "portable.h"
#include "types.h"

// Divides p_a, shifted left by 29, by p_b.
int32_t FixedDiv29(int32_t p_a, int32_t p_b)
{
	return PortableIdiv((int64_t) p_a * 0x20000000, p_b);
}
