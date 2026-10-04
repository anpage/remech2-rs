#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "inputdriver.h"
#include "types.h"

#include <windows.h>

// The functions and globals of keyboard.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern InputDriverModule g_keyboardDriver;

	void HandleKeyboardMessages(UINT p_msg, WPARAM p_wParam, LPARAM p_lParam);
	void KeyboardClearKeyStates(void);
	MechS16 KeyboardPollKeyCode(void);

#ifdef __cplusplus
}
#endif

#endif // KEYBOARD_H
