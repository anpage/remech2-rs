#ifndef RESOURCENAME_H
#define RESOURCENAME_H

#include "resourceref.h"
#include "types.h"

// The functions of resourcename.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 FindResourceIdByName(MechS32 p_table, MechChar* p_name);
	void* LoadResourceByRef(
		ResourceRef* p_ref,
		const char* p_type,
		const char* p_ext,
		MechS32 p_table,
		MechS32* p_size,
		MechU32* p_poolTag
	);

#ifdef __cplusplus
}
#endif

#endif // RESOURCENAME_H
