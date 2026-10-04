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

#ifdef __cplusplus
}
#endif

#endif // MOUSE_H
