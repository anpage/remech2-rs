#ifndef MOUSE_H
#define MOUSE_H

#include "decomp.h"
#include "inputdriver.h"
#include "types.h"

// The functions and globals of mouse.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern undefined4 g_reclipCursor;
	extern InputDriverModule g_mouseDriver;
	extern MechChar* g_mouseAxisNames[2];
	extern MechChar* g_mouseAxisTypes[2];
	extern MechChar* g_mouseButtonNames[4];
	extern MechChar* g_mouseButtonTypes[3];
	extern MechS32 g_cursorClipped;
	extern MechChar g_mouseDeviceName[8];
	extern MechChar g_mouseDisplayName[8];
	extern MechChar g_mouseTypeName[8];

	MechS32 MousePoll(void* p_data, MechS32* p_position, MechU32* p_buttons);

#ifdef __cplusplus
}
#endif

#endif // MOUSE_H
