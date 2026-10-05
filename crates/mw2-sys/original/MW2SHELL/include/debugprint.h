#ifndef DEBUGPRINT_H
#define DEBUGPRINT_H

#include "types.h"

// The functions and globals of debugprint.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechChar g_debugPrintBuffer[0x100];

	void ShowMessage(const MechChar* p_format, ...);
	void DebugPrint(const MechChar* p_format, ...);

#ifdef __cplusplus
}
#endif

#endif // DEBUGPRINT_H
