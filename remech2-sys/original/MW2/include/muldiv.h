#ifndef MULDIV_H
#define MULDIV_H

#include "types.h"

// Implemented on the Rust side (src/sim/math.rs). The original was hand-written assembly.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 MulDiv64(MechS32 p_a, MechS32 p_b, MechS32 p_c);

#ifdef __cplusplus
}
#endif

#endif // MULDIV_H
