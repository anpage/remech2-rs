#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

#include <windows.h>

// The functions and globals of keyboard.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void HandleKeyboardMessages(UINT p_msg, WPARAM p_wParam, LPARAM p_lParam);
	MechS16 KeyboardPollKeyCode(void);

#ifdef __cplusplus
}
#endif

#endif // KEYBOARD_H
