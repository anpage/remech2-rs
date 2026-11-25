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
	extern int32_t g_cursorClipped;

	void MouseRelease(void);
	int32_t MousePoll(void* p_data, int32_t* p_position, uint32_t* p_buttons);

#ifdef __cplusplus
}
#endif

#endif // MOUSE_H
