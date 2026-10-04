/* Hand-written assembly: ApproximateVectorLength is a C function whose body is an __asm block
   (the /Od frame saves ebx/esi/edi, which the body never touches). Its portable C (PORTABLE_C) is
   tested against the assembly by tests/asmequiv. */
#include "approxlen.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Approximates the length of (p_x, p_y, p_z) as (max * 4 + the other two) / 4 of the absolute
// values.
// FUNCTION: MW2 0x100035c0
MechS32 ApproximateVectorLength(MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
#ifdef PORTABLE_C
	/* neg leaves INT_MIN negative, and the comparisons are signed; the sum wraps. */
	MechS32 x = p_x < 0 ? PortableS32(0 - (MechU32) p_x) : p_x;
	MechS32 y = p_y < 0 ? PortableS32(0 - (MechU32) p_y) : p_y;
	MechS32 z = p_z < 0 ? PortableS32(0 - (MechU32) p_z) : p_z;
	MechS32 swap;

	if (x < y) {
		swap = x;
		x = y;
		y = swap;
	}

	if (x < z) {
		swap = x;
		x = z;
		z = swap;
	}

	return PortableSar32(PortableS32(((MechU32) x << 2) + (MechU32) y + (MechU32) z), 2);
#else
	__asm {
		mov eax, p_x
		mov edx, p_y
		mov ecx, p_z
		cmp eax, 0
		jge x_positive
		neg eax
x_positive:
		cmp edx, 0
		jge y_positive
		neg edx
y_positive:
		cmp ecx, 0
		jge z_positive
		neg ecx
z_positive:
		cmp eax, edx
		jge x_above_y
		xchg eax, edx
x_above_y:
		cmp eax, ecx
		jge x_above_z
		xchg eax, ecx
x_above_z:
		shl eax, 2
		add eax, edx
		add eax, ecx
		sar eax, 2
	}
#endif
}
