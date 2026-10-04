/* Hand-written assembly: Lerp is a C function whose body is an __asm block, like
   MulDiv64. Its portable C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "lerp.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Interpolates linearly: the y at p_x on the line through (p_x0, p_y0) and (p_x1, p_y1), measured
// from whichever end has the larger x.
// FUNCTION: MW2 0x100349f0
MechS32 Lerp(MechS32 p_x0, MechS32 p_x1, MechS32 p_x, MechS32 p_y0, MechS32 p_y1)
{
#ifdef PORTABLE_C
	/* The differences and the sum wrap at 32 bits. */
	MechS32 run;
	MechS64 rise;
	MechS32 base;

	if (p_x1 > p_x0) {
		run = PortableS32((MechU32) p_x0 - (MechU32) p_x1);
		rise = (MechS64) PortableS32((MechU32) p_x - (MechU32) p_x1) * PortableS32((MechU32) p_y0 - (MechU32) p_y1);
		base = p_y1;
	}
	else {
		run = PortableS32((MechU32) p_x1 - (MechU32) p_x0);
		rise = (MechS64) PortableS32((MechU32) p_x - (MechU32) p_x0) * PortableS32((MechU32) p_y1 - (MechU32) p_y0);
		base = p_y0;
	}

	return PortableS32((MechU32) PortableIdiv(rise, run) + (MechU32) base);
#else
	__asm {
		mov ebx, p_x0
		mov ecx, p_x1
		mov eax, p_x
		mov edi, p_y0
		mov edx, p_y1
		cmp ecx, ebx
		jg jmp_10034a1e
		sub ecx, ebx
		sub eax, ebx
		sub edx, edi
		imul edx
		idiv ecx
		add eax, edi
		jmp jmp_10034a2c
jmp_10034a1e:
		sub ebx, ecx
		sub eax, ecx
		mov ecx, edx
		sub edi, ecx
		imul edi
		idiv ebx
		add eax, ecx
jmp_10034a2c:
	}
#endif
}
