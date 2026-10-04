/* Hand-written assembly: FixedDot29 is a C function whose body is an __asm block. Its portable C
   (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "fixeddot29.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Returns p_a * p_b + p_c * p_d + p_e * p_f, shifted right by 29 and rounded.
// FUNCTION: MW2 0x10042700
MechS32 FixedDot29(MechS32 p_a, MechS32 p_b, MechS32 p_c, MechS32 p_d, MechS32 p_e, MechS32 p_f)
{
#ifdef PORTABLE_C
	/* The sum wraps at 64 bits, like the add/adc chain. */
	MechU64 sum = (MechU64) ((MechS64) p_a * p_b) + (MechU64) ((MechS64) p_c * p_d) + (MechU64) ((MechS64) p_e * p_f);

	return PortableS32(PortableShrdRound(sum, 29));
#else
	__asm {
		mov eax, p_a
		mov edx, p_b
		mov ecx, p_c
		mov ebx, p_d
		mov esi, p_e
		mov edi, p_f
		push esi
		push edi
		imul edx
		mov esi, eax
		mov edi, edx
		mov eax, ecx
		imul ebx
		add esi, eax
		adc edi, edx
		pop eax
		pop ebx
		imul ebx
		add esi, eax
		adc edi, edx
		shrd esi, edi, 29
		adc esi, 0
		mov eax, esi
	}
#endif
}
