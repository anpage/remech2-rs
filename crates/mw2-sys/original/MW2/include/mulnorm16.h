#ifndef MULNORM16_H
#define MULNORM16_H

#include "types.h"

// The functions and globals of mulnorm16.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void MulNormalize16(MechS32* p_low, MechS32* p_scaled, MechS16* p_shift, MechU32 p_a, MechU32 p_b);

#ifdef __cplusplus
}
#endif

#endif // MULNORM16_H
