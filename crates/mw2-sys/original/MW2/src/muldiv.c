/* Hand-written assembly: MulDiv64 is a C function whose body is an __asm block (the /Od frame
   saves esi/edi, which the body never touches). Its portable C (PORTABLE_C) is tested against the
   assembly by tests/asmequiv. */
#include "muldiv.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// FUNCTION: MW2 0x100035a0
MechS32 MulDiv64(MechS32 p_a, MechS32 p_b, MechS32 p_c)
{
#ifdef PORTABLE_C
	return PortableIdiv((MechS64) p_a * p_b, p_c);
#else
	__asm {
		mov eax, p_a
		mov edx, p_b
		mov ecx, p_c
		imul edx
		idiv ecx
	}
#endif
}
