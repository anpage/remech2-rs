#ifndef RESOURCECACHE_H
#define RESOURCECACHE_H

#include "decomp.h"
#include "types.h"

// The functions and globals of resourcecache.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void InitializeResourceCache(void);
	void UnlockCachedResource(MechS32 p_id, char* p_type);
	void* LoadCachedResource(MechS32 p_handle, MechS32 p_id, char* p_type, MechS32 p_unk0x0c);
	void* AllocateMemory(undefined4 p_size);
	void ShutdownResourceCache(void);
	void FreeCachedResource(MechS32 p_id, char* p_type);

#ifdef __cplusplus
}
#endif

#endif // RESOURCECACHE_H
