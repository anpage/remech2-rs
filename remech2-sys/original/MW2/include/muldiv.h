#ifndef MULDIV_H
#define MULDIV_H

#include "types.h"

// Implemented on the Rust side (src/sim/math.rs). The original was hand-written assembly.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t MulDiv64(int32_t p_a, int32_t p_b, int32_t p_c);

#ifdef __cplusplus
}
#endif

#endif // MULDIV_H
