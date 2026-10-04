#include "debugprint.h"

#include "debugout.h"
#include "refreshmode.h"
#include "types.h"

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

// GLOBAL: MW2SHELL 0x10096760
MechChar g_debugPrintBuffer[0x100];

// FUNCTION: MW2SHELL 0x10015c90
void ShowMessage(const MechChar* p_format, ...)
{
	va_list args;

	va_start(args, p_format);
	_vsnprintf(g_debugPrintBuffer, sizeof(g_debugPrintBuffer), p_format, args);
	va_end(args);
	OutputDebugString(g_debugPrintBuffer);
	MessageBox(g_gameWindow, g_debugPrintBuffer, "MechWarrior2 Message", MB_ICONASTERISK);
}

// FUNCTION: MW2SHELL 0x10015ce8
void DebugPrint(const MechChar* p_format, ...)
{
	va_list args;

	va_start(args, p_format);
	_vsnprintf(g_debugPrintBuffer, sizeof(g_debugPrintBuffer), p_format, args);
	va_end(args);
	DebugPrintInternal(g_debugPrintBuffer);
}
