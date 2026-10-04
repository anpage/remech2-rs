/* Hand-written assembly: MulAddDiv is a C function whose body is an __asm block. Its portable
   C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "muladddiv.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Returns (p_a * p_b + (p_c << 16)) / p_d, with a 64-bit intermediate.
// FUNCTION: MW2 0x1004c860
MechS32 MulAddDiv(MechS32 p_a, MechS32 p_b, MechS32 p_c, MechS32 p_d)
{
#ifdef PORTABLE_C
	return PortableIdiv((MechS64) p_a * p_b + (MechS64) p_c * 0x10000, p_d);
#else
	__asm {
		mov eax, p_a
		mov edx, p_b
		mov ebx, p_c
		mov ecx, p_d
		imul edx
		mov edi, edx
		mov esi, eax
		mov eax, ebx
		cdq
		shld edx, eax, 16
		shl eax, 16
		add eax, esi
		adc edx, edi
		idiv ecx
	}
#endif
}
