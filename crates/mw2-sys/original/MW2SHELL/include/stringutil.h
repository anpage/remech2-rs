#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include "types.h"

// The functions and globals of stringutil.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechChar* AllocateString(MechChar* p_string);
	MechChar* UppercaseString(MechChar* p_string);

#ifdef __cplusplus
}
#endif

#endif // STRINGUTIL_H
