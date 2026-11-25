#ifndef MULNORM16_H
#define MULNORM16_H

#include "types.h"

// The functions and globals of mulnorm16.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void MulNormalize16(int32_t* p_low, int32_t* p_scaled, int16_t* p_shift, uint32_t p_a, uint32_t p_b);

#ifdef __cplusplus
}
#endif

#endif // MULNORM16_H
