/* Hand-written assembly: ProjectRadius is a C function whose body is an __asm block. Its portable
   C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "muldiv14.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Returns p_scale * p_radius / p_depth / 2^14, the product kept in 64 bits and shifted by 8 before the division
// and the quotient by 6 after it.
// FUNCTION: MW2 0x10013340
MechS32 ProjectRadius(MechS32 p_scale, MechS32 p_radius, MechS32 p_depth)
{
#ifdef PORTABLE_C
	MechS64 scaled = PortableSar64((MechS64) p_scale * p_radius, 8);

	return PortableSar32(PortableIdiv(scaled, p_depth), 6);
#else
	__asm {
		mov eax, p_scale
		mov edx, p_radius
		mov ecx, p_depth
		imul edx
		shrd eax, edx, 8
		sar edx, 8
		idiv ecx
		sar eax, 6
	}
#endif
}
