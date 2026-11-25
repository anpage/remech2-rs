#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include "types.h"

// The functions and globals of stringutil.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	char* AllocateString(char* p_string);
	char* UppercaseString(char* p_string);

#ifdef __cplusplus
}
#endif

#endif // STRINGUTIL_H
