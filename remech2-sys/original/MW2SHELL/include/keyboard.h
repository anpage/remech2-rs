#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "decomp.h"
#include "types.h"

#include <stddef.h>

enum KeyCodeBuffer {
	c_keyCodeBufferSize = 64
};

// The functions and globals of keyboard.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern undefined4 g_keyCodeWriteIndex;
	extern undefined4 g_keyCodeReadIndex;
	extern MechS16 g_keyCodes[c_keyCodeBufferSize];
	extern undefined4 g_keyStates[4];
	extern MechS16 g_keyCodeMap[256];
	extern MechS32 g_extendedScanCodeMap[0x59];
	extern MechChar* g_keyShortNames[0x79];
	extern MechChar* g_keyNames[0x79];

	void HandleKeyboardMessages(MechU32 p_msg, size_t p_wParam, MECH_INTPTR p_lParam);
	MechS16 KeyboardPollKeyCode(void);

#ifdef __cplusplus
}
#endif

#endif // KEYBOARD_H
