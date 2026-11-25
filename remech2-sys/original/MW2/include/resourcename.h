#ifndef RESOURCENAME_H
#define RESOURCENAME_H

#include "resourceref.h"
#include "types.h"

// The functions of resourcename.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t FindResourceIdByName(int32_t p_table, char* p_name);
	void* LoadResourceByRef(
		ResourceRef* p_ref,
		const char* p_type,
		const char* p_ext,
		int32_t p_table,
		int32_t* p_size,
		uint32_t* p_poolTag
	);

#ifdef __cplusplus
}
#endif

#endif // RESOURCENAME_H
