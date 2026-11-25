#ifndef ERROR_H
#define ERROR_H

#include "types.h"

#include <stdarg.h>

// The functions and globals of error.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern char* g_fatalErrorTitle;
	extern char* g_warningTitle;
	extern int32_t g_errorCode;
	extern char g_errorMessage[0x400];

	void Error(int32_t p_code, const char* p_format, ...);
	void ShutdownOnError(void);
	void ShowFatalError(const char* p_format, va_list p_args);
	char* FormatErrorMessage(char* p_title, int32_t p_code, const char* p_format, va_list p_args);
	void LogWarning(const char* p_format, va_list p_args);

#ifdef __cplusplus
}
#endif

#endif // ERROR_H
