#ifndef FIXEDDIV_H
#define FIXEDDIV_H

#include "types.h"

// Implemented on the Rust side (src/sim/math.rs). The original was hand-written assembly.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t FixedDiv16(int32_t p_a, int32_t p_b);

#ifdef __cplusplus
}
#endif

#endif // FIXEDDIV_H
