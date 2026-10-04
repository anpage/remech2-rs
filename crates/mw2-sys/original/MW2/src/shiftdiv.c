/* Hand-written assembly: ProjectCoordinate is a C function whose body is an __asm block. Its portable
   C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "shiftdiv.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Returns (p_value << p_shift) / p_depth, divided by 4 and rounded, plus p_center.
// FUNCTION: MW2 0x10042740
MechS32 ProjectCoordinate(MechS32 p_value, MechS32 p_depth, MechS32 p_shift, MechS32 p_center)
{
#ifdef PORTABLE_C
	/* shld/shl take the count modulo 32; the rounding and the sum wrap. */
	MechU32 quotient = (MechU32) PortableIdiv((MechS64) p_value * ((MechS64) 1 << (p_shift & 31)), p_depth);

	return PortableS32((MechU32) PortableSar32(PortableS32(quotient + 2), 2) + (MechU32) p_center);
#else
	__asm {
		mov eax, p_value
		mov esi, p_depth
		mov ecx, p_shift
		mov ebx, p_center
		cdq
		shld edx, eax, cl
		shl eax, cl
		idiv esi
		add eax, 2
		sar eax, 2
		add eax, ebx
	}
#endif
}
