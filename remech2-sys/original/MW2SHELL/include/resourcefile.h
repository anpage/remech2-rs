#ifndef RESOURCEFILE_H
#define RESOURCEFILE_H

#include "types.h"

// The functions and globals of resourcefile.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechChar g_resourceDir[0x100];
	extern MechChar g_resourcePath[0x50];

	MechChar* MakeResourcePath(MechChar* p_name);

#ifdef __cplusplus
}
#endif

#endif // RESOURCEFILE_H
