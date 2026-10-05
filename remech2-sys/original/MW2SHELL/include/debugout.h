#ifndef DEBUGOUT_H
#define DEBUGOUT_H

#include "types.h"

#include <stdio.h>

// The functions and globals of debugout.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_debugOutputMode;
	extern FILE* g_debugLogFile;
	extern MechChar g_debugLogName[0x100];

	MechS32 SetDebugOutputMode(MechS32 p_mode);
	void DebugPrintInternal(MechChar* p_message, ...);

#ifdef __cplusplus
}
#endif

#endif // DEBUGOUT_H
