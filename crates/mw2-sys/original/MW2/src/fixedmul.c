/* Hand-written assembly: FixedMul16 is a C function whose body is an __asm block (the /Od frame
   saves esi/edi, which the body never touches). Its portable C (PORTABLE_C) is tested against the
   assembly by tests/asmequiv. */
#include "fixedmul.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// FUNCTION: MW2 0x10003580
MechS32 FixedMul16(MechS32 p_a, MechS32 p_b)
{
#ifdef PORTABLE_C
	return PortableS32(PortableShrdRound((MechU64) ((MechS64) p_a * p_b), 16));
#else
	__asm {
		mov eax, p_a
		mov ebx, p_b
		imul ebx
		shrd eax, edx, 16
		adc eax, 0
	}
#endif
}
