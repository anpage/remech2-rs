#ifndef KEYBOARDINPUT_H
#define KEYBOARDINPUT_H

#include "decomp.h"
#include "font.h"
#include "types.h"

// Keyboard input for the shell: the last key code polled, and a line of edited text.
// SIZE 0x110
class KeyboardInput {
public:
	KeyboardInput();
	~KeyboardInput();

	void FlushKeys();
	MechS32 WaitForKey();
	undefined4 GetKeyPressed();
	MechS32 PollKey();
	void ResetText(undefined4 p_maxLength);
	void SetText(undefined4 p_maxLength, const MechChar* p_text);
	MechS32 EditText(MechChar* p_text, MechU8 p_upperCase);

	// The text entry loop reads the key directly: an inline accessor would leave a jmp at /Ob1.
	MechU32 m_key; // 0x00

private:
	MechS32 m_length;        // 0x04
	MechS32 m_maxLength;     // 0x08
	MechChar m_text[0x100];  // 0x0c
	undefined4 m_keyPressed; // 0x10c — whether PollKey read a key
};

// The functions and globals of keyboardinput.cpp that other units use.
extern MechChar g_editTextBuffer[0x100];
MechS32 EditTextField(
	Font* p_font,
	MechS32 p_left,
	MechS32 p_top,
	MechChar* p_text,
	undefined* p_colors,
	MechS32 p_maxLength,
	MechS32 p_width
);

#endif // KEYBOARDINPUT_H
