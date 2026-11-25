#ifndef RESOURCENAME_H
#define RESOURCENAME_H

#include "types.h"

// The functions and globals of resourcename.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif
	int32_t FindResourceIdByName(int32_t p_type, char* p_name);

#ifdef __cplusplus
}
#endif

#endif // RESOURCENAME_H
