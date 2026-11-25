/* In the original, MulAddDiv is a C function whose body is an __asm block. This is portable C in
   its place. */
#include "muladddiv.h"

#include "portable.h"
#include "types.h"

// Returns (p_a * p_b + (p_c << 16)) / p_d, with a 64-bit intermediate.
int32_t MulAddDiv(int32_t p_a, int32_t p_b, int32_t p_c, int32_t p_d)
{
	return PortableIdiv((int64_t) p_a * p_b + (int64_t) p_c * 0x10000, p_d);
}
