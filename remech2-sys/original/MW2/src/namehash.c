/* In the original, HashName is a C function whose body is an __asm block. This is portable C in its
   place. */
#include "namehash.h"

#include "portable.h"
#include "types.h"

// Hashes a name, ignoring case: each character is added and the low word rotated left.
uint32_t HashName(const char* p_name)
{
	uint32_t hash = 0;
	uint32_t low;

	while (*p_name) {
		/* The add carries into the high word; only the low word rotates. */
		hash += (uint8_t) *p_name++ | 0x20;
		low = hash & 0xffff;
		hash = (hash & 0xffff0000) | (((low << 1) | (low >> 15)) & 0xffff);
	}

	return hash;
}
