#ifndef DEBUGOUT_H
#define DEBUGOUT_H

#include "types.h"

// The functions and globals of debugout.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 SetDebugOutputMode(MechS32 p_mode);
	void DebugPrintInternal(MechChar* p_message, ...);

#ifdef __cplusplus
}
#endif

#endif // DEBUGOUT_H
