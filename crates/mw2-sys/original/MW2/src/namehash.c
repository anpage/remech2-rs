/* In the original, HashName is a C function whose body is an __asm block. This is portable C in its
   place. */
#include "namehash.h"

#include "portable.h"
#include "types.h"

// Hashes a name, ignoring case: each character is added and the low word rotated left.
// FUNCTION: MW2 0x100074e0
MechU32 HashName(const MechChar* p_name)
{
	MechU32 hash = 0;
	MechU32 low;

	while (*p_name) {
		/* The add carries into the high word; only the low word rotates. */
		hash += (MechU8) *p_name++ | 0x20;
		low = hash & 0xffff;
		hash = (hash & 0xffff0000) | (((low << 1) | (low >> 15)) & 0xffff);
	}

	return hash;
}
