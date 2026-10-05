#include "debugprint.h"

#include "debugout.h"
#include "log.h"
#include "types.h"

#include <stdarg.h>
#include <stdio.h>

// GLOBAL: MW2SHELL 0x10096760
MechChar g_debugPrintBuffer[0x100];

// FUNCTION: MW2SHELL 0x10015c90
void ShowMessage(const MechChar* p_format, ...)
{
	va_list args;

	va_start(args, p_format);
	vsnprintf(g_debugPrintBuffer, sizeof(g_debugPrintBuffer), p_format, args);
	va_end(args);
	// The original sent it to the debugger and showed it in a message box
	MechLogError(g_debugPrintBuffer);
}

// FUNCTION: MW2SHELL 0x10015ce8
void DebugPrint(const MechChar* p_format, ...)
{
	va_list args;

	va_start(args, p_format);
	vsnprintf(g_debugPrintBuffer, sizeof(g_debugPrintBuffer), p_format, args);
	va_end(args);
	DebugPrintInternal(g_debugPrintBuffer);
}
