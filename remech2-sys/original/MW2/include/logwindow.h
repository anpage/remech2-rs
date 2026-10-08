#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include "types.h"

#include <stdio.h>

// The functions and globals of logwindow.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void DebugPrintInternal(const MechChar* p_format, ...);

#ifdef __cplusplus
}
#endif

#endif // LOGWINDOW_H
