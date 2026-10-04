#ifndef ERROR_H
#define ERROR_H

#include "types.h"

// The functions and globals of error.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void Error(MechS32 p_code, const char* p_format, ...);
	void ShutdownOnError(void);
	void ShowFatalError(const char** p_args);
	MechChar* FormatErrorMessage(MechChar* p_title, MechS32 p_code, const char** p_args);
	void LogWarning(const char** p_args);

#ifdef __cplusplus
}
#endif

#endif // ERROR_H
