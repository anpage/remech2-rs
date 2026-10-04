#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include "types.h"

#include <stdio.h>
#include <windows.h>

// The functions and globals of logwindow.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_debugOutputMode;
	extern HWND g_debugWindow;
	extern UINT g_debugMessageBoxType;
	extern FILE* g_debugLogFile;
	extern MechChar g_debugLogName[0x100];
	extern MechChar g_debugMessageTitle[0x50];

	void ScrollMonoDisplay(void);
	void ClearMonoLastLine(void);
	void PrintMonoLine(MechChar* p_text);
	void PrintMono(MechChar* p_text);
	void CreateDebugLog(void);
	void AppendDebugLog(MechChar* p_text);
	MechS32 SetDebugOutputMode(MechS32 p_mode);
	void SetDebugWindow(HWND p_window);
	void SetDebugMessageBoxType(UINT p_type);
	void SetDebugMessageTitle(const MechChar* p_format, ...);
	void SetDebugLogName(const MechChar* p_path);
	void DebugPrintInternal(const MechChar* p_format, ...);

#ifdef __cplusplus
}
#endif

#endif // LOGWINDOW_H
