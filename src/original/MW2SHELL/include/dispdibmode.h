#ifndef DISPDIBMODE_H
#define DISPDIBMODE_H

#include "displaybackend.h"
#include "refreshmode.h"
#include "types.h"

// The functions and globals of dispdib.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern DisplayBackend g_dispDibBackend;
	extern RefreshMode g_dispDibRefreshMode;

#ifdef __cplusplus
}
#endif

#endif // DISPDIBMODE_H
