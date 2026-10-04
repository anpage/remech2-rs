#ifndef RESOURCENAME_H
#define RESOURCENAME_H

#include "decomp.h"
#include "types.h"

// The functions and globals of resourcename.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif
	MechS32 FindResourceIdByName(MechS32 p_type, char* p_name);

#ifdef __cplusplus
}
#endif

#endif // RESOURCENAME_H
