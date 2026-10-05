#include "debugout.h"

#include "decomp.h"
#include "files.h"
#include "log.h"
#include "types.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Where DebugPrintInternal sends messages: 1 the monochrome display (text memory at 0xb0000),
// 2 the debugger, 4 the log file; anything else drops them. In the original 3 was a message box,
// shown only once SetDebugWindow was called, which nothing did.
// GLOBAL: MW2SHELL 0x10064b70
MechS32 g_debugOutputMode = 0;

// GLOBAL: MW2SHELL 0x10064b7c
FILE* g_debugLogFile = NULL;

// GLOBAL: MW2SHELL 0x10064b80
MechChar g_debugLogName[0x100] = "debug.log";

// FUNCTION: MW2SHELL 0x10017710
void ScrollMonoDisplay(void)
{
	// The ranges overlap (dst 0xb0000 < src 0xb00a0), so this is memmove: /Oi would expand
	// memcpy inline even at this constant size.
	memmove((void*) 0xb0000, (void*) 0xb00a0, 0xf00);
}

// FUNCTION: MW2SHELL 0x10017732
void ClearMonoLastLine(void)
{
	MechS32 pixelAddress = 0xb0f00;

	while (pixelAddress < 0xb0fa0) {
		*(MechS32*) MECH_S32_TO_PTR(pixelAddress) = *(MechS32*) MECH_S32_TO_PTR(pixelAddress) & 0xff00ff00;
		pixelAddress += 4;
	}
}

// Stack-slot permutation: original source/destination/length/index are at
// [ebp-0xc]/[ebp-4]/[ebp-0x10]/[ebp-8]; VC++ assigns
// [ebp-4]/[ebp-0x10]/[ebp-8]/[ebp-0xc] here.
// FUNCTION: MW2SHELL 0x1001776c
void PrintMonoLine(MechChar* p_message)
{
	MechChar* source = p_message;
	MechChar* destination = (MechChar*) 0xb0f00;
	MechS32 length = strlen(p_message);
	MechS32 index;

	if (length > 0x50) {
		length = 0x50;
	}

	ScrollMonoDisplay();
	ClearMonoLastLine();
	for (index = 0; index < length; index++) {
		*destination = *source;
		source++;
		destination++;
		destination++;
	}
}

// Stack-slot permutation: original length/offset are at [ebp-8]/[ebp-4];
// VC++ assigns [ebp-4]/[ebp-8] here.
// FUNCTION: MW2SHELL 0x100177e9
void PrintMono(MechChar* p_message)
{
	MechS32 length = strlen(p_message);
	MechS32 offset = 0;

	while (offset < length) {
		PrintMonoLine(p_message + offset);
		offset += 0x50;
	}
}

// FUNCTION: MW2SHELL 0x10017836
void CreateDebugLog(void)
{
	g_debugLogFile = MechFopen(g_debugLogName, "wt");
}

// FUNCTION: MW2SHELL 0x10017858
void AppendDebugLog(MechChar* p_message)
{
	if (g_debugLogFile == NULL) {
		g_debugLogFile = MechFopen(g_debugLogName, "at");
	}
	if (g_debugLogFile != NULL) {
		fputs(p_message, g_debugLogFile);
		fflush(g_debugLogFile);
		fclose(g_debugLogFile);
		g_debugLogFile = NULL;
	}
}

// FUNCTION: MW2SHELL 0x100178cc
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

// FUNCTION: MW2SHELL 0x10017961
void SetDebugLogName(MechChar* p_fileName)
{
	strncpy(g_debugLogName, p_fileName, 0x100);
}

// Stack-slot permutation: original message is at [ebp-0x100] and args at [ebp-0x104];
// VC++ assigns them [ebp-0x104] and [ebp-4] here.
// FUNCTION: MW2SHELL 0x10017982
void DebugPrintInternal(MechChar* p_message, ...)
{
	MechChar message[0x100];
	va_list args;

	va_start(args, p_message);
	vsnprintf(message, sizeof(message), p_message, args);
	va_end(args);

	switch (g_debugOutputMode) {
	case 1:
		PrintMono(message);
		break;
	case 2:
		MechLogDebug(message);
		break;
	case 4:
		AppendDebugLog(message);
		break;
	}
}
