/* Hand-written assembly: FixedDiv16 is a C function whose body is an __asm block, like
   FixedMul16. Its portable C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "fixeddiv.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Divides two 16.16 fixed-point values (no check for a zero divisor).
// FUNCTION: MW2 0x10002c90
MechS32 FixedDiv16(MechS32 p_a, MechS32 p_b)
{
#ifdef PORTABLE_C
	return PortableIdiv((MechS64) p_a * 0x10000, p_b);
#else
	__asm {
		mov eax, p_a
		mov ebx, p_b
		cdq
		shld edx, eax, 16
		shl eax, 16
		idiv ebx
	}
#endif
}
