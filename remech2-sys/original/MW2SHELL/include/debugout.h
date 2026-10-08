#ifndef DEBUGOUT_H
#define DEBUGOUT_H

#include "types.h"

#include <stdio.h>

// The functions and globals of debugout.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void DebugPrintInternal(MechChar* p_message, ...);

#ifdef __cplusplus
}
#endif

#endif // DEBUGOUT_H
