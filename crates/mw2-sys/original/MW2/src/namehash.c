/* Hand-written assembly: HashName is a C function whose body is an __asm block. Its portable
   C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "namehash.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* no return value: the result is left in eax */

// Hashes a name, ignoring case: each character is added and the low word rotated left.
// FUNCTION: MW2 0x100074e0
MechU32 HashName(const MechChar* p_name)
{
#ifdef PORTABLE_C
	MechU32 hash = 0;
	MechU32 low;

	while (*p_name) {
		/* The add carries into the high word; only the low word rotates. */
		hash += (MechU8) *p_name++ | 0x20;
		low = hash & 0xffff;
		hash = (hash & 0xffff0000) | (((low << 1) | (low >> 15)) & 0xffff);
	}

	return hash;
#else
	__asm {
		mov edx, p_name
		xor eax, eax
		jmp check
next:
		xor ebx, ebx
		mov bl, [edx]
		or bl, 0x20
		add eax, ebx
		inc edx
		rol ax, 1
check:
		cmp byte ptr [edx], 0
		jne next
	}
#endif
}
