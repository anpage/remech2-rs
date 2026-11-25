#ifndef DEBUGPRINT_H
#define DEBUGPRINT_H

#include "types.h"

// The functions and globals of debugprint.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern char g_debugPrintBuffer[0x100];

	void ShowMessage(const char* p_format, ...);
	void DebugPrint(const char* p_format, ...);

#ifdef __cplusplus
}
#endif

#endif // DEBUGPRINT_H
