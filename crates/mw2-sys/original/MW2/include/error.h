#ifndef ERROR_H
#define ERROR_H

#include "types.h"

// The functions and globals of error.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechChar* g_fatalErrorTitle;
	extern MechChar* g_warningTitle;
	extern MechS32 g_errorCode;
	extern MechChar g_errorMessage[0x400];

	void Error(MechS32 p_code, const char* p_format, ...);
	void ShutdownOnError(void);
	void ShowFatalError(const char** p_args);
	MechChar* FormatErrorMessage(MechChar* p_title, MechS32 p_code, const char** p_args);
	void LogWarning(const char** p_args);

#ifdef __cplusplus
}
#endif

#endif // ERROR_H
