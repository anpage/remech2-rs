#ifndef KEYBOARDINPUT_H
#define KEYBOARDINPUT_H

#include "font.h"
#include "types.h"

// Keyboard input for the shell: the last key code polled, and a line of edited text.
class KeyboardInput {
public:
	KeyboardInput();
	~KeyboardInput();

	void FlushKeys();
	int32_t WaitForKey();
	uint32_t GetKeyPressed();
	int32_t PollKey();
	void ResetText(uint32_t p_maxLength);
	void SetText(uint32_t p_maxLength, const char* p_text);
	int32_t EditText(char* p_text, uint8_t p_upperCase);

	uint32_t m_key;

private:
	int32_t m_length;
	int32_t m_maxLength;
	char m_text[0x100];
	uint32_t m_keyPressed; // whether PollKey read a key
};

// The functions and globals of keyboardinput.cpp that other units use.
extern char g_editTextBuffer[0x100];
int32_t EditTextField(
	Font* p_font,
	int32_t p_left,
	int32_t p_top,
	char* p_text,
	uint8_t* p_colors,
	int32_t p_maxLength,
	int32_t p_width
);

#endif // KEYBOARDINPUT_H
