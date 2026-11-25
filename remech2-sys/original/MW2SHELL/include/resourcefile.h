#ifndef RESOURCEFILE_H
#define RESOURCEFILE_H

#include "types.h"

// The functions and globals of resourcefile.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern char g_resourceDir[0x100];
	extern char g_resourcePath[0x50];

	char* MakeResourcePath(char* p_name);

#ifdef __cplusplus
}
#endif

#endif // RESOURCEFILE_H
