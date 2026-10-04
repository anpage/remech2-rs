/* Hand-written assembly: MulNormalize16 is a C function whose body is an __asm block. Its portable
   C (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "mulnorm16.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

// Multiplies p_a by p_b (unsigned) into *p_low; *p_scaled gets the product shifted right until it
// fits in 16 bits, and *p_shift is increased by the shift.
// FUNCTION: MW2 0x1004c820
void MulNormalize16(MechS32* p_low, MechS32* p_scaled, MechS16* p_shift, MechU32 p_a, MechU32 p_b)
{
#ifdef PORTABLE_C
	/* bsr of a zero low dword leaves the cleared ecx as it is (the processors' behaviour, which
	   the xor before it relies on): no shift. */
	MechU64 product = (MechU64) p_a * p_b;
	MechU32 low = (MechU32) product;
	MechS32 shift;

	*p_low = PortableS32(low);
	if (low != 0) {
		shift = PortableBsr(low) - 15;
		if (shift > 0) {
			*p_shift = PortableS16((MechU16) ((MechU16) *p_shift + shift));
			low = (MechU32) (product >> shift);
		}
	}

	*p_scaled = PortableS32(low);
#else
	__asm {
		mov esi, p_low
		mov edi, p_scaled
		mov edx, p_shift
		mov ebx, p_a
		mov eax, p_b
		push edx
		mul ebx
		mov [esi], eax
		pop ebx
		xor ecx, ecx
		bsr ecx, eax
		sub cx, 15
		jle done
		add word ptr [ebx], cx
		shrd eax, edx, cl
done:
		mov [edi], eax
	}
#endif
}
