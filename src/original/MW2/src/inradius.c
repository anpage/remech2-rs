/* Hand-written assembly: IsWithinRadius is a C function whose body is an __asm block. Its portable
   C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "inradius.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Returns 1 if p_x^2 + p_y^2 + p_z^2 (64-bit) is at most p_radius^2.
// FUNCTION: MW2 0x10004ec0
MechS32 IsWithinRadius(MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_radius)
{
#ifdef PORTABLE_C
	/* The squares are compared unsigned, and a sum at or above the radius's square counts as
	   inside when their difference's low dword is 0. */
	MechU64 sum = (MechU64) ((MechS64) p_x * p_x) + (MechU64) ((MechS64) p_y * p_y) + (MechU64) ((MechS64) p_z * p_z);
	MechU64 square = (MechU64) ((MechS64) p_radius * p_radius);

	return sum < square || (MechU32) (sum - square) == 0;
#else
	__asm {
		mov eax, p_x
		mov edi, p_y
		mov ecx, p_z
		mov ebx, p_radius
		imul eax
		mov esi, eax
		mov eax, edi
		mov edi, edx
		imul eax
		add esi, eax
		adc edi, edx
		mov eax, ecx
		imul eax
		add esi, eax
		adc edi, edx
		mov eax, ebx
		imul eax
		sub esi, eax
		sbb edi, edx
		jnc not_below
		mov eax, 1
		jmp done
not_below:
		cmp esi, 0
		jne outside
		mov eax, 1
		jmp done
outside:
		mov eax, 0
done:
	}
#endif
}
