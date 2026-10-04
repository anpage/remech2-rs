#ifndef FIXEDDIV_H
#define FIXEDDIV_H

#include "types.h"

// Implemented on the Rust side (src/sim/math.rs). The original was hand-written assembly.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 FixedDiv16(MechS32 p_a, MechS32 p_b);

#ifdef __cplusplus
}
#endif

#endif // FIXEDDIV_H
