/* Hand-written assembly: FixedSqrtGuess is a C function whose body is an __asm block. Its
   portable C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "sqrtguess.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Estimates the square root of a 16.16 fixed-point value from its highest set bit, for
// FixedSqrt16 to refine.
// FUNCTION: MW2 0x10016a90
MechU32 FixedSqrtGuess(MechU32 p_value)
{
#ifdef PORTABLE_C
	/* bsr of 0 leaves the shift undefined */
	MechS32 bit;

	PORTABLE_ASSERT(p_value != 0);
	bit = PortableBsr(p_value);

	if (bit > 15) {
		return p_value >> ((bit - 15) >> 1);
	}

	return p_value << ((16 - bit) >> 1);
#else
	__asm {
		mov eax, p_value
		bsr ecx, eax
		sub ecx, 15
		jle small
		shr ecx, 1
		shr eax, cl
		jmp done
small:
		bsr cx, ax
		neg cx
		add cx, 16
		shr cx, 1
		shl eax, cl
done:
	}
#endif
}
