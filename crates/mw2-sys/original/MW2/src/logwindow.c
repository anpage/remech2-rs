#include "logwindow.h"

#include "decomp.h"
#include "types.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

// The debug log: messages go to a monochrome monitor's text screen (the MDA frame buffer at
// 0xb0000, 80 by 25), the debugger, a message box or a log file, as g_debugOutputMode selects.

// GLOBAL: MW2 0x100a5730
MechS32 g_debugOutputMode = 0;

// GLOBAL: MW2 0x100a5734
HWND g_debugWindow = NULL;

// GLOBAL: MW2 0x100a5738
UINT g_debugMessageBoxType = MB_ICONINFORMATION;

// GLOBAL: MW2 0x100a573c
FILE* g_debugLogFile = NULL;

// GLOBAL: MW2 0x100a5740
MechChar g_debugLogName[0x100] = "debug.log";

// GLOBAL: MW2 0x100a5840
MechChar g_debugMessageTitle[0x50] = "DEBUG Message";

// Scrolls the monochrome screen up a line.
// FUNCTION: MW2 0x1003a1c0
void ScrollMonoDisplay(void)
{
	memmove((void*) 0xb0000, (void*) 0xb00a0, 0xf00);
}

// Clears the characters of the monochrome screen's last line, keeping their attributes.
// FUNCTION: MW2 0x1003a1e2
void ClearMonoLastLine(void)
{
	MechS32* cell;

	cell = (MechS32*) 0xb0f00;
	while ((MechS32) cell < 0xb0fa0) {
		*cell &= 0xff00ff00;
		cell++;
	}
}

// Stack-slot permutation: cell, i, text and length.
// FUNCTION: MW2 0x1003a21c
void PrintMonoLine(MechChar* p_text)
{
	MechChar* cell;
	MechS32 i;
	MechChar* text;
	MechS32 length;

	text = p_text;
	cell = (MechChar*) 0xb0f00;
	length = strlen(p_text);
	if (length > 0x50) {
		length = 0x50;
	}

	ScrollMonoDisplay();
	ClearMonoLastLine();
	for (i = 0; i < length; i++) {
		*cell++ = *text++;
		cell++;
	}
}

// Stack-slot permutation: i and length.
// FUNCTION: MW2 0x1003a299
void PrintMono(MechChar* p_text)
{
	MechS32 i;
	MechS32 length;

	length = strlen(p_text);
	i = 0;
	while (i < length) {
		PrintMonoLine(p_text + i);
		i += 0x50;
	}
}

// FUNCTION: MW2 0x1003a2e6
void CreateDebugLog(void)
{
	g_debugLogFile = fopen(g_debugLogName, "wt");
}

// FUNCTION: MW2 0x1003a308
void AppendDebugLog(MechChar* p_text)
{
	if (g_debugLogFile == NULL) {
		g_debugLogFile = fopen(g_debugLogName, "at");
	}

	if (g_debugLogFile != NULL) {
		fputs(p_text, g_debugLogFile);
		fflush(g_debugLogFile);
		fclose(g_debugLogFile);
		g_debugLogFile = NULL;
	}
}

// FUNCTION: MW2 0x1003a37c
MechS32 SetDebugOutputMode(MechS32 p_mode)
{
	if (p_mode >= 0 && p_mode <= 4) {
		g_debugOutputMode = p_mode;
		return 0;
	}
	else {
		return 2;
	}
}

// FUNCTION: MW2 0x1003a3b9
void SetDebugWindow(HWND p_window)
{
	g_debugWindow = p_window;
}

// FUNCTION: MW2 0x1003a3cc
void SetDebugMessageBoxType(UINT p_type)
{
	g_debugMessageBoxType = p_type;
}

// FUNCTION: MW2 0x1003a3df
void SetDebugMessageTitle(const MechChar* p_format, ...)
{
	va_list args;

	va_start(args, p_format);
	_vsnprintf(g_debugMessageTitle, sizeof(g_debugMessageTitle), p_format, args);
	va_end(args);
}

// FUNCTION: MW2 0x1003a411
void SetDebugLogName(const MechChar* p_path)
{
	strncpy(g_debugLogName, p_path, sizeof(g_debugLogName));
}

// Stack-slot permutation: message and args (which moves the jump table targets).
// FUNCTION: MW2 0x1003a432
void DebugPrintInternal(const MechChar* p_format, ...)
{
	MechChar message[0x100];
	va_list args;

	va_start(args, p_format);
	_vsnprintf(message, sizeof(message), p_format, args);
	va_end(args);

	switch (g_debugOutputMode) {
	case 1:
		PrintMono(message);
		break;
	case 2:
		OutputDebugString(message);
		break;
	case 4:
		AppendDebugLog(message);
		break;
	case 3:
		if (g_debugWindow) {
			MessageBox(g_debugWindow, message, g_debugMessageTitle, g_debugMessageBoxType);
		}
		break;
	default:
		break;
	}
}
