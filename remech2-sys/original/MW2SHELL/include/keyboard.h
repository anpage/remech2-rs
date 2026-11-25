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
	extern int16_t g_keyCodes[c_keyCodeBufferSize];
	extern undefined4 g_keyStates[4];
	extern int16_t g_keyCodeMap[256];
	extern int32_t g_extendedScanCodeMap[0x59];

	void HandleKeyboardMessages(uint32_t p_msg, size_t p_wParam, MECH_INTPTR p_lParam);
	int32_t KeyboardFlushKeyCodes(void);
	int16_t KeyboardPollKeyCode(void);

#ifdef __cplusplus
}
#endif

#endif // KEYBOARD_H
