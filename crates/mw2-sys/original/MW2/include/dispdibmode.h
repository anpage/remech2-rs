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

// The DisplayDib SDK header's __inline helpers, which dispdib.c calls without expanding them: the
// compiler emits them after its functions, each in its own 16-byte aligned section.

// FUNCTION: MW2 0x1004f2d0
// DisplayDibWindowMessage

// FUNCTION: MW2 0x1004f330
// DisplayDibWindowCreateEx

#endif // DISPDIBMODE_H
