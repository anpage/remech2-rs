/* Hand-written assembly: FixedMul29 is a C function whose body is an __asm block. Its portable
   C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "fixedmul29.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// p_a * p_b >> 29, rounded.
// FUNCTION: MW2 0x10019ad0
MechS32 FixedMul29(MechS32 p_a, MechS32 p_b)
{
#ifdef PORTABLE_C
	return PortableS32(PortableShrdRound((MechU64) ((MechS64) p_a * p_b), 29));
#else
	__asm {
		mov eax, p_a
		mov edx, p_b
		imul edx
		shrd eax, edx, 29
		adc eax, 0
	}
#endif
}
