#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

#include <stddef.h>

// The functions and globals of keyboard.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void HandleKeyboardMessages(MechU32 p_msg, size_t p_wParam, MECH_INTPTR p_lParam);
	MechS16 KeyboardPollKeyCode(void);

#ifdef __cplusplus
}
#endif

#endif // KEYBOARD_H
