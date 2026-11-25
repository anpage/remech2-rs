#ifndef NAMEHASH_H
#define NAMEHASH_H

#include "types.h"

// The functions and globals of namehash.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	uint32_t HashName(const char* p_name);

#ifdef __cplusplus
}
#endif

#endif // NAMEHASH_H
