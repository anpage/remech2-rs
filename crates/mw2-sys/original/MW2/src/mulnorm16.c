/* In the original, MulNormalize16 is a C function whose body is an __asm block. This is portable C
   in its place. */
#include "mulnorm16.h"

#include "portable.h"
#include "types.h"

// Multiplies p_a by p_b (unsigned) into *p_low; *p_scaled gets the product shifted right until it
// fits in 16 bits, and *p_shift is increased by the shift.
// FUNCTION: MW2 0x1004c820
void MulNormalize16(MechS32* p_low, MechS32* p_scaled, MechS16* p_shift, MechU32 p_a, MechU32 p_b)
{
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
}
