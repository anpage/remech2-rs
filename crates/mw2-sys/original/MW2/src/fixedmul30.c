/* Hand-written assembly: FixedMul30 is a C function whose body is an __asm block, like
   FixedMul16. Its portable C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "fixedmul30.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Multiplies two 2.30 fixed-point values, rounding.
// FUNCTION: MW2 0x10044720
MechS32 FixedMul30(MechS32 p_a, MechS32 p_b)
{
#ifdef PORTABLE_C
	return PortableS32(PortableShrdRound((MechU64) ((MechS64) p_a * p_b), 30));
#else
	__asm {
		mov eax, p_a
		mov edx, p_b
		imul edx
		shrd eax, edx, 30
		adc eax, 0
	}
#endif
}
