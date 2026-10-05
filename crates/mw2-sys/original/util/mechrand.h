#ifndef MECHRAND_H
#define MECHRAND_H

// The C library's rand and srand with the original's (Visual C++'s) 15-bit range, implemented on
// the Rust side (src/mech_rand.rs). The game assumes that range; glibc's rand returns 31 bits.
// The numbers aren't Visual C++'s.

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

// The largest MechRand returns
#define MECH_RAND_MAX 0x7fff

	// Seeds the sequence, like srand: the same seed gives the same numbers
	void MechSRand(MechU32 p_seed);
	// The next number of the sequence, 0 to MECH_RAND_MAX, like rand
	MechS32 MechRand(void);

#ifdef __cplusplus
}
#endif

#endif // MECHRAND_H
