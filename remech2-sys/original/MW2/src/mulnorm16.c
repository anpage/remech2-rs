/* In the original, MulNormalize16 is a C function whose body is an __asm block. This is portable C
   in its place. */
#include "mulnorm16.h"

#include "portable.h"
#include "types.h"

// Multiplies p_a by p_b (unsigned) into *p_low; *p_scaled gets the product shifted right until it
// fits in 16 bits, and *p_shift is increased by the shift.
void MulNormalize16(int32_t* p_low, int32_t* p_scaled, int16_t* p_shift, uint32_t p_a, uint32_t p_b)
{
	/* bsr of a zero low dword leaves the cleared ecx as it is (the processors' behaviour, which
	   the xor before it relies on): no shift. */
	MechU64 product = (MechU64) p_a * p_b;
	uint32_t low = (uint32_t) product;
	int32_t shift;

	*p_low = PortableS32(low);
	if (low != 0) {
		shift = PortableBsr(low) - 15;
		if (shift > 0) {
			*p_shift = PortableS16((uint16_t) ((uint16_t) *p_shift + shift));
			low = (uint32_t) (product >> shift);
		}
	}

	*p_scaled = PortableS32(low);
}
