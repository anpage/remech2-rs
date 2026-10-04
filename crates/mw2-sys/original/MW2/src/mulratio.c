/* Hand-written assembly: MulRatio is a C function whose body is an __asm block. Its portable
   C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "mulratio.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// p_a * p_b / (p_b + p_c) >> 12.
// FUNCTION: MW2 0x1004c800
MechS32 MulRatio(MechS32 p_a, MechS32 p_b, MechS32 p_c)
{
#ifdef PORTABLE_C
	/* The divisor wraps at 32 bits, and the quotient is shifted unsigned. */
	MechS32 divisor = PortableS32((MechU32) p_b + (MechU32) p_c);

	return (MechS32) ((MechU32) PortableIdiv((MechS64) p_a * p_b, divisor) >> 12);
#else
	__asm {
		mov eax, p_a
		mov ebx, p_b
		mov ecx, p_c
		imul ebx
		add ebx, ecx
		idiv ebx
		shr eax, 12
	}
#endif
}
