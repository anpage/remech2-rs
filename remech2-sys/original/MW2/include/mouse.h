#ifndef MOUSE_H
#define MOUSE_H

#include "decomp.h"
#include "types.h"

// The functions and globals of mouse.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern undefined4 g_reclipCursor;
	extern MechS32 g_cursorClipped;

	void MouseRelease(void);
	MechS32 MousePoll(void* p_data, MechS32* p_position, MechU32* p_buttons);

#ifdef __cplusplus
}
#endif

#endif // MOUSE_H
