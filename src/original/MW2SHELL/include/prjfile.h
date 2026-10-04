#ifndef PRJFILE_H
#define PRJFILE_H

#include "decomp.h"
#include "types.h"

// The functions and globals of prjfile.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void SetArchiveAllocator(void* (*p_alloc)(undefined4), void (*p_free)(void*));
	MechS32 OpenArchive(char* p_name, MechChar p_mode);
	MechS32 CloseArchive(MechS32 p_handle);
	MechS32 LoadArchiveEntries(MechS32 p_handle);
	MechS32 GetArchiveItemSize(MechS32 p_handle, MechChar* p_name, MechU16 p_index);
	MechS32 ReadArchiveItem(MechS32 p_handle, MechChar* p_name, MechU16 p_index, void* p_data);

#ifdef __cplusplus
}
#endif

#endif // PRJFILE_H
