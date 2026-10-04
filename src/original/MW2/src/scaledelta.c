/* Hand-written assembly: UnprojectCoordinate is a C function whose body is an __asm block, like
   MulDiv64. Its portable C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "scaledelta.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Scales the difference p_screen - p_center by 4 * p_depth, as a 64-bit product shifted right by p_shift.
// FUNCTION: MW2 0x10034990
MechS32 UnprojectCoordinate(MechS32 p_screen, MechS32 p_center, MechS32 p_depth, MechS32 p_shift)
{
#ifdef PORTABLE_C
	/* The difference and its scaling wrap at 32 bits; shrd takes the count modulo 32. */
	MechS32 difference = PortableS32(((MechU32) p_screen - (MechU32) p_center) << 2);

	return PortableS32((MechU32) ((MechU64) ((MechS64) difference * p_depth) >> (p_shift & 31)));
#else
	__asm {
		mov eax, p_screen
		mov edx, p_center
		mov ebx, p_depth
		mov ecx, p_shift
		sub eax, edx
		shl eax, 2
		imul ebx
		shrd eax, edx, cl
	}
#endif
}
