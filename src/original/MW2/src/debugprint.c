#include "debugprint.h"

#include "logwindow.h"
#include "simmain.h"
#include "types.h"

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

// GLOBAL: MW2 0x100ea3f0
MechChar g_debugPrintBuffer[0x100];

// FUNCTION: MW2 0x10050900
void ShowMessage(const MechChar* p_format, ...)
{
	va_list args;

	va_start(args, p_format);
	_vsnprintf(g_debugPrintBuffer, sizeof(g_debugPrintBuffer), p_format, args);
	va_end(args);
	OutputDebugString(g_debugPrintBuffer);
	MessageBox(g_gameWindow, g_debugPrintBuffer, "MechWarrior2 Message", MB_ICONASTERISK);
}

// FUNCTION: MW2 0x10050958
void DebugPrint(const MechChar* p_format, ...)
{
	va_list args;

	va_start(args, p_format);
	_vsnprintf(g_debugPrintBuffer, sizeof(g_debugPrintBuffer), p_format, args);
	va_end(args);
	DebugPrintInternal(g_debugPrintBuffer);
}
