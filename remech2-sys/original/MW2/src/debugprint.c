#include "debugprint.h"

#include "log.h"
#include "logwindow.h"
#include "types.h"

#include <stdarg.h>
#include <stdio.h>

char g_debugPrintBuffer[0x100];

void ShowMessage(const char* p_format, ...)
{
	va_list args;

	va_start(args, p_format);
	vsnprintf(g_debugPrintBuffer, sizeof(g_debugPrintBuffer), p_format, args);
	va_end(args);
	// The original sent it to the debugger and showed it in a message box
	MechLogError(g_debugPrintBuffer);
}

void DebugPrint(const char* p_format, ...)
{
	va_list args;

	va_start(args, p_format);
	vsnprintf(g_debugPrintBuffer, sizeof(g_debugPrintBuffer), p_format, args);
	va_end(args);
	DebugPrintInternal(g_debugPrintBuffer);
}
