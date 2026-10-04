#include "error.h"

#include "bwdnames.h"
#include "clock.h"
#include "debugprint.h"
#include "decomp.h"
#include "inifile.h"
#include "inputmap.h"
#include "loadres.h"
#include "log.h"
#include "mw2log.h"
#include "network.h"
#include "overlay.h"
#include "pointer.h"
#include "render.h"
#include "resource.h"
#include "simmain.h"
#include "types.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// GLOBAL: MW2 0x100a589c
MechChar* g_fatalErrorTitle = "\nMW2.EXE - fatal error";

// GLOBAL: MW2 0x100a58a0
MechChar* g_warningTitle = "\nMW2.EXE - warning";

// The code of the last error.
// GLOBAL: MW2 0x100a58a4
MechS32 g_errorCode = 0;

// The error message; the length is unknown.
// GLOBAL: MW2 0x100be010
MechChar g_errorMessage[0x400];

// Reports error p_code, with a printf-style message: codes that are warnings are logged, the
// others are fatal and end the game, most of them after shutting the subsystems down.
// FUNCTION: MW2 0x1003b8c0
void Error(MechS32 p_code, const char* p_format, ...)
{
	g_errorCode = p_code;
	switch (g_errorCode) {
	case 1:
	case 8:
	case 10:
	case 0x12:
	case 0x16:
	case 0x17:
	case 0x1d:
	case 0x1e:
	case 0x1f:
	case 0x20:
	case 0x25:
	case 0x27:
	case 0x28:
	case 0x2c:
	case 0x2d:
	case 0x2e:
	case 0x2f:
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x3d:
	case 0x46:
	case 0x47:
	case 0x4b:
		LogWarning(&p_format);
		break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 9:
	case 0xb:
	case 0xd:
	case 0xe:
	case 0xf:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x18:
	case 0x19:
	case 0x1a:
	case 0x1b:
	case 0x1c:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x26:
	case 0x29:
	case 0x2a:
	case 0x2b:
	case 0x35:
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3e:
	case 0x3f:
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x48:
	case 0x49:
	case 0x4a:
	case 0x4d:
	case 0x4e:
	case 0x4f:
	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
		ShutdownOnError();
	case 0x10:
	case 0x11:
	case 0x44:
	case 0x45:
	case 0x4c:
	case 0x54:
		ShowFatalError(&p_format);
		break;
	default:
		break;
	}
}

// Shuts the subsystems down before the game exits on an error.
// FUNCTION: MW2 0x1003ba07
void ShutdownOnError(void)
{
	FreeBwdNames();
	FreeMissionTables();
	ShutdownResourceCache();
	ShutdownNetwork();
	CloseInputDevices();
	CloseMw2Log();
	ShutdownRender();
	StopTimers();
}

// Shows a fatal error's message, logs it, and exits.
// FUNCTION: MW2 0x1003ba3a
void ShowFatalError(const char** p_args)
{
	ShutdownRender();
	StopTimers();
	CloseInputDevices();
	MechMouseShowCursor(TRUE);

	// The original minimised the window and showed it in a message box
	MechLogError(FormatErrorMessage(g_fatalErrorTitle, g_errorCode, p_args));
	DebugPrint(FormatErrorMessage(g_fatalErrorTitle, g_errorCode, p_args));
	exit(g_errorCode);
}

// Formats an error's message: p_title, the code, the code's text from the "SystemError" section,
// and the printf-style message p_args points at, if any. The message is logged and shown.
// Stack-slot permutation of code and args.
// FUNCTION: MW2 0x1003bad1
MechChar* FormatErrorMessage(MechChar* p_title, MechS32 p_code, const char** p_args)
{
	MechChar code[4];
	va_list args;

	sprintf(code, "%02X", p_code);
	FindIniSection("SystemError");
	sprintf(g_errorMessage, "%s #%02X: %s", p_title, p_code, GetIniValue(code));
	if (*p_args) {
		args = (va_list) (p_args + 1);
		strcat(g_errorMessage, ": ");
		vsprintf(g_errorMessage + strlen(g_errorMessage), *p_args, args);
		va_end(args);
	}

	WriteToMw2Log(g_errorMessage);
	MonoPrint(g_errorMessage);
	return g_errorMessage;
}

// Logs a warning's message and clears the error code.
// FUNCTION: MW2 0x1003bbb1
void LogWarning(const char** p_args)
{
	FormatErrorMessage(g_warningTitle, g_errorCode, p_args);
	g_errorCode = 0;
}
