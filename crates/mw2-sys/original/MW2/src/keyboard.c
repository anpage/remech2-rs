/* The keyboard object is shared with MW2SHELL.DLL (every function has the same size, and
   KeyboardReadKeyCode is byte-identical); both targets keep their own copy until one source
   matches both. */
#include "keyboard.h"

#include "decomp.h"
#include "inputdeviceinfo.h"
#include "inputdriver.h"
#include "messages.h"
#include "types.h"

#include <ctype.h>
#include <stdio.h>

// Key code modifier bits.
enum KeyCodeModifier {
	c_keyCodeControl = 0x100,
	c_keyCodeShift = 0x200,
	c_keyCodeAlt = 0x400
};

// The modifier bits: the state bits of the virtual keys 118 to 120, "Any Shift Key", "Any
// Control Key" and "Any Alt Key", in word c_modifierWord of g_keyStates.
enum KeyModifier {
	c_modifierShift = 0x400000,
	c_modifierControl = 0x800000,
	c_modifierAlt = 0x1000000
};

// The word of g_keyStates that holds the modifier bits.
enum KeyStateWord {
	c_modifierWord = 3
};

// GLOBAL: MW2 0x100be5d0
undefined4 g_keyCodeWriteIndex;

// GLOBAL: MW2 0x100be5cc
undefined4 g_keyCodeReadIndex;

// A ring buffer of key codes, filled by KeyboardQueueKeyCode.
// GLOBAL: MW2 0x10109a30
MechS16 g_keyCodes[c_keyCodeBufferSize];

// One bit per (remapped) scan code, set while the key is down. Word 3 also holds the modifier
// bits (see KeyModifier): the original keeps them in the same memory, so clearing the states
// clears the modifiers too, and a copy of the states carries them.
// GLOBAL: MW2 0x10109a20
undefined4 g_keyStates[4];

// clang-format off
// The key code for each virtual key; 0 falls back to MapVirtualKey.
// GLOBAL: MW2 0x100a6308
MechS16 g_keyCodeMap[256] = {
	0,     0,     0,     0,     0,     0,     0,     0,     0x08,  0x09,  0,     0,     0,     0x0d,  0,     0,
	0,     0,     0,     0x1ff, 0,     0,     0,     0,     0,     0,     0,     0x1b,  0,     0,     0,     0,
	0x20,  0xc4,  0xc5,  0xc3,  0xc2,  0xc9,  0xc6,  0xc8,  0xc7,  0,     0,     0,     0,     0xc0,  0xc1,  0,
	0x30,  0x31,  0x32,  0x33,  0x34,  0x35,  0x36,  0x37,  0x38,  0x39,  0,     0,     0,     0,     0,     0,
	0,     0x61,  0x62,  0x63,  0x64,  0x65,  0x66,  0x67,  0x68,  0x69,  0x6a,  0x6b,  0x6c,  0x6d,  0x6e,  0x6f,
	0x70,  0x71,  0x72,  0x73,  0x74,  0x75,  0x76,  0x77,  0x78,  0x79,  0x7a,  0,     0,     0,     0,     0,
	0xc0,  0xc3,  0xc7,  0xc5,  0xc9,  0,     0xc8,  0xc2,  0xc6,  0xc4,  0xcb,  0xcd,  0,     0xcc,  0x2e,  0xca,
	0xb1,  0xb2,  0xb3,  0xb4,  0xb5,  0xb6,  0xb7,  0xb8,  0xb9,  0xba,  0xbb,  0xbc,
};
// clang-format on

// clang-format off
// The key state index of the extended (0xe0-prefixed) scan codes.
// GLOBAL: MW2 0x100a6508
MechS32 g_extendedScanCodeMap[0x59] = {
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0x55,  0x70,  0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0x59,  0,     0,     0x71,  0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0x45,  0,     0x5e,  0x5f,  0x60,  0,     0x61,  0,     0x62,  0,     0x63,
	0x64,  0x65,  0x66,  0x67,  0,     0,     0,     0x57,  0x58,
};
// clang-format on

// Two entries of each key-name table share an empty name that precedes the table's strings in
// the original, instead of an empty literal of their own. Like the strings, it is const data.
// Being the empty string is their only role, so they keep their placeholders.
// GLOBAL: MW2 0x1009d150
const MechChar g_unk0x1009d150[] = "";

// GLOBAL: MW2 0x1009d54c
const MechChar g_unk0x1009d54c[] = "";

// clang-format off
// The short name of each key, as INPUT.MAP writes it.
// GLOBAL: MW2 0x100a5f38
MechChar* g_keyShortNames[0x79] = {
	"*Esc", "*One", "*Two", "*Three", "*Four", "*Five", "*Six", "*Seven", "*Eight", "*Nine", "*Zero", "Minus", "Equal",
	"*Backspace", "Tab", "*Q", "*W", "*E", "*R", "*T", "Y", "*U", "*I", "*O", "*P", "LeftBracket", "RightBracket",
	"Enter", "LeftControl", "*A", "*S", "D", "*F", "G", "H", "J", "*K", "*L", "*Semicolon", "*Quote", "*BackQuote",
	"LeftShift", "*BackSlash", "Z", "*X", "*C", "*V", "*B", "*N", "*M", "Comma", "Period", "*Slash", "RightShift",
	"GreyStar", "", "Space", "CAPSLock", "*F1", "*F2", "*F3", "*F4", "*F5", "*F6", "*F7", "*F8", "*F9", "*F10",
	"NUMLock", "ScrollLock", "Home", "UpArrow", "PageUp", "GreyMinus", "LeftArrow", "Keypad5", "RightArrow", "GreyPlus",
	"End", "DownArrow", "PageDown", "Insert", "Delete", "SYSREQ", "KeypadEnter", "LeftBackSlash", "*F11", "F12",
	"GreySlash", "PA1", "F13", "F14", "F15", "GreyHome", "GreyUpArrow", "GreyPageUp", "GreyLeftArrow", "GreyRightArrow",
	"GreyEnd", "GreyDownArrow", "GreyPageDown", "GreyInsert", "GreyDelete", "F21", "F22", "F23", "F24", "UNNAMED_1",
	"EraseEOF", (MechChar*) g_unk0x1009d150, "CopyPlay", "RightCtrl", "", "CRSel", (MechChar*) g_unk0x1009d150, "EXSel", "UNAMED_2", "Clear",
	"Shift", "Control", "",
};
// clang-format on

// clang-format off
// The display name of each key.
// GLOBAL: MW2 0x100a6120
MechChar* g_keyNames[0x79] = {
	"Escape Key", "Number One", "Number Two", "Number Three", "Number Four", "Number Five", "Number Six",
	"Number Seven", "Number Eight", "Number Nine", "Number Zero", "Minus (-) Key", "Equal (=) Key", "Backspace Key",
	"Tab Key", "Q Key", "W Key", "E Key", "R Key", "T Key", "Y Key", "U Key", "I Key", "O Key", "P Key",
	"Left Bracket ([) Key", "Right Bracket (]) Key", "Enter (CR) Key", "Left Control Key", "A Key", "S Key", "D Key",
	"F Key", "G Key", "H Key", "J Key", "K Key", "L Key", "Semicolon (;) Key", "Quote (\") Key", "Back Quote (`) Key",
	"Left Shift Key", "Back Slash (\\) Key", "Z Key", "X Key", "C Key", "V Key", "B Key", "N Key", "M Key",
	"Comma (,) Key", "Period (.) Key", "Slash (/) Key", "Right Shift Key", "Grey Star (*) Key", "Left Alt Key",
	"Spacebar", "CAPS Lock Key", "F1 Key", "F2 Key", "F3 Key", "F4 Key", "F5 Key", "F6 Key", "F7 Key", "F8 Key",
	"F9 Key", "F10 Key", "NUM Lock Key", "Scroll Lock Key", "Home Key", "Up Arrow Key", "Page Up Key",
	"Grey Minus (-) Key", "Left Arrow Key", "Keypad 5 Key", "Right Arrow Key", "Grey Plus (+) Key", "End Key",
	"Down Arrow Key", "Page Down Key", "Insert Key", "Delete Key", "SYSREQ Key", "Keypad Enter Key",
	"Left Back Slash (\\) Key", "F11 Key", "F12 Key", "Grey Slash (/) Key", "PA1 Key", "F13 Key", "F14 Key", "F15 Key",
	"Grey Home Key", "Grey Up Arrow Key", "Grey Page Up Key", "Grey Left Arrow Key", "Grey Right Arrow Key",
	"Grey End Key", "Grey Down Arrow Key", "Grey Page Down Key", "Grey Insert Key", "Grey Delete Key", "F21 Key",
	"F22 Key", "F23 Key", "F24 Key", "UNNAMED Key", "Erase EOF Key", (MechChar*) g_unk0x1009d54c, "Copy Play Key",
	"Right Control Key", "Right Alt Key", "CR Sel Key", (MechChar*) g_unk0x1009d54c, "EX Sel Key", "UNAMED_2 Key", "Clear Key",
	"Any Shift Key", "Any Control Key", "Any Alt Key",
};
// clang-format on

MechS32 GetKeyboardDeviceCount(void);
MechS32 FillKeyboardDeviceInfo(MechS32 p_index, InputDeviceInfo* p_info);
MechS32 KeyboardOpenDevice(void);
MechS32 KeyboardCloseDevice(void);
MechS32 KeyboardCenterAxis(void);
MechS32 KeyboardPoll(undefined4 p_unk0x00, undefined4 p_unk0x04, undefined4* p_keyStates);
MechS32 KeyboardReadKeyCode(MechS16* p_keyCode);
MechS32 KeyboardFlushKeyCodes(void);
void KeyboardQueueKeyCode(size_t p_virtualKey, MECH_INTPTR p_lParam);
void KeyboardRecordKeyState(size_t p_virtualKey, MechU32 p_lParam, MechS32 p_pressed);

// GLOBAL: MW2 0x100a6670
InputDriverModule g_keyboardDriver = {
	GetKeyboardDeviceCount,
	FillKeyboardDeviceInfo,
	KeyboardOpenDevice,
	KeyboardCloseDevice,
	KeyboardCenterAxis,
	KeyboardPoll,
	KeyboardReadKeyCode,
	KeyboardFlushKeyCodes,
};

// FUNCTION: MW2 0x10042770
MechS32 GetKeyboardDeviceCount(void)
{
	return 1;
}

// FUNCTION: MW2 0x10042785
MechS32 FillKeyboardDeviceInfo(MechS32 p_index, InputDeviceInfo* p_info)
{
	p_info->m_axisCount = 0;
	p_info->m_buttonCount = 0x79;
	sprintf(p_info->m_shortName, "keyboard");
	sprintf(p_info->m_displayName, "Keyboard");
	sprintf(p_info->m_matchName, "keyboard");
	p_info->m_axisNames = NULL;
	p_info->m_axisShortNames = NULL;
	p_info->m_buttonNames = g_keyNames;
	p_info->m_buttonShortNames = g_keyShortNames;
	p_info->m_driverData = NULL;
	return 0;
}

// FUNCTION: MW2 0x10042816
MechS32 KeyboardOpenDevice(void)
{
	return 0;
}

// FUNCTION: MW2 0x10042828
MechS32 KeyboardCloseDevice(void)
{
	return 0;
}

// FUNCTION: MW2 0x1004283a
MechS32 KeyboardCenterAxis(void)
{
	return 0;
}

// FUNCTION: MW2 0x1004284c
MechS32 KeyboardPoll(undefined4 p_unk0x00, undefined4 p_unk0x04, undefined4* p_keyStates)
{
	MechS32 i;

	if (!(g_keyStates[c_modifierWord] & c_modifierAlt) && p_keyStates) {
		for (i = 0; i < 4; i++) {
			p_keyStates[i] = g_keyStates[i];
		}
	}

	return 0;
}

// FUNCTION: MW2 0x100428a9
MechS32 KeyboardReadKeyCode(MechS16* p_keyCode)
{
	if (g_keyCodeReadIndex == g_keyCodeWriteIndex) {
		*p_keyCode = 0;
	}
	else {
		*p_keyCode = g_keyCodes[g_keyCodeReadIndex];
		g_keyCodeReadIndex++;
		if (g_keyCodeReadIndex == c_keyCodeBufferSize) {
			g_keyCodeReadIndex = 0;
		}
	}

	return 0;
}

// FUNCTION: MW2 0x10042909
MechS32 KeyboardFlushKeyCodes(void)
{
	MechMessage msg;

	if (MechPeekMessage(&msg, c_mechMsgKeyFirst, c_mechMsgKeyLast, TRUE)) {
		HandleKeyboardMessages(msg.m_message, msg.m_wParam, msg.m_lParam);
	}

	g_keyCodeReadIndex = g_keyCodeWriteIndex = 0;
	return 0;
}

// Unlike the shell's, the simulator queues a key's code when the key is released.
// FUNCTION: MW2 0x10042966
void HandleKeyboardMessages(MechU32 p_msg, size_t p_wParam, MECH_INTPTR p_lParam)
{
	MechMessage msg;
	MechS32 done;

	done = FALSE;
	do {
		switch (p_msg) {
		case c_mechMsgKeyDown:
		case c_mechMsgSysKeyDown:
			KeyboardRecordKeyState(p_wParam, p_lParam, TRUE);
			break;
		case c_mechMsgKeyUp:
		case c_mechMsgSysKeyUp:
			KeyboardQueueKeyCode(p_wParam, p_lParam);
			KeyboardRecordKeyState(p_wParam, p_lParam, FALSE);
			break;
		}

		// The original passed system key messages on to DefWindowProc, and the next message through
		// TranslateMessage.
		if (MechPeekMessage(&msg, c_mechMsgKeyFirst, c_mechMsgKeyLast, TRUE)) {
			p_msg = msg.m_message;
			p_wParam = msg.m_wParam;
			p_lParam = msg.m_lParam;
		}
		else {
			done = TRUE;
		}
	} while (!done);
}

// FUNCTION: MW2 0x10042a71
void KeyboardQueueKeyCode(size_t p_virtualKey, MECH_INTPTR p_lParam)
{
	MechS16 keyCode;

	keyCode = g_keyCodeMap[p_virtualKey];
	if (keyCode == 0) {
		// The original asked MapVirtualKey for the key's character.
		keyCode = (MechChar) (p_lParam & 0xff);
		keyCode = tolower(keyCode);
	}

	if (keyCode != 0) {
		// The original asked GetKeyState for the modifiers.
		if (g_keyStates[c_modifierWord] & c_modifierControl) {
			keyCode |= c_keyCodeControl;
		}
		if (g_keyStates[c_modifierWord] & c_modifierAlt) {
			keyCode |= c_keyCodeAlt;
		}
		if (g_keyStates[c_modifierWord] & c_modifierShift) {
			keyCode |= c_keyCodeShift;
		}

		g_keyCodes[g_keyCodeWriteIndex] = keyCode;
		g_keyCodeWriteIndex++;
		if (g_keyCodeWriteIndex == c_keyCodeBufferSize) {
			g_keyCodeWriteIndex = 0;
		}
		else if (g_keyCodeReadIndex == g_keyCodeWriteIndex) {
			g_keyCodeReadIndex++;
			if (g_keyCodeReadIndex == c_keyCodeBufferSize) {
				g_keyCodeReadIndex = 0;
			}
		}
	}
}

// FUNCTION: MW2 0x10042b90
void KeyboardRecordKeyState(size_t p_virtualKey, MechU32 p_lParam, MechS32 p_pressed)
{
	MechS32 key;

	key = (p_lParam >> 16) & 0xff;
	if (key != 0 && ((p_lParam >> 16) & 0x100) && key < 0x59) {
		key = g_extendedScanCodeMap[key];
	}

	if (key != 0) {
		key--;
		if (p_pressed) {
			g_keyStates[key / 32] |= 1 << (key & 0x1f);
		}
		else {
			g_keyStates[key / 32] &= ~(1 << (key & 0x1f));
		}

		switch (key) {
		case 0x29:
		case 0x35:
			if (p_pressed) {
				g_keyStates[c_modifierWord] |= c_modifierShift;
			}
			else {
				g_keyStates[c_modifierWord] &= ~c_modifierShift;
			}
			break;
		case 0x1c:
		case 0x6f:
			if (p_pressed) {
				g_keyStates[c_modifierWord] |= c_modifierControl;
			}
			else {
				g_keyStates[c_modifierWord] &= ~c_modifierControl;
			}
			break;
		case 0x37:
		case 0x70:
			if (p_pressed) {
				g_keyStates[c_modifierWord] |= c_modifierAlt;
			}
			else {
				g_keyStates[c_modifierWord] &= ~c_modifierAlt;
			}
			break;
		}
	}
}

// FUNCTION: MW2 0x10042d55
void KeyboardClearKeyStates(void)
{
	MechS32 i;

	for (i = 0; i < 4; i++) {
		g_keyStates[i] = 0;
	}
}

// FUNCTION: MW2 0x10042d8f
MechS16 KeyboardPollKeyCode(void)
{
	MechMessage msg;
	MechS16 keyCode;

	KeyboardReadKeyCode(&keyCode);
	if (keyCode == 0 && MechPeekMessage(&msg, c_mechMsgKeyFirst, c_mechMsgKeyLast, TRUE)) {
		HandleKeyboardMessages(msg.m_message, msg.m_wParam, msg.m_lParam);
		KeyboardReadKeyCode(&keyCode);
	}

	return keyCode;
}
