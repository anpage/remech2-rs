#include "keyboardinput.h"

#include "font.h"
#include "keyboard.h"
#include "mousestate.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "stringutil.h"
#include "textglyph.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <ctype.h>
#include <string.h>

enum {
	c_keyBackspace = 0x08,
	c_keyReturn = 0x0d,
	c_keyEscape = 0x1b,
	c_keyTilde = 0x7e,
	c_keyCodeControl = 0x100,
	c_keyCodeShift = 0x200,
	c_keyCodeAlt = 0x400
};

MechChar g_editTextBuffer[0x100];

KeyboardInput::KeyboardInput()
{
	m_key = 0;
	ResetText(0x100);
	FlushKeys();
}

KeyboardInput::~KeyboardInput()
{
}

void KeyboardInput::FlushKeys()
{
	KeyboardFlushKeyCodes();
	m_key = 0;
}

MechS32 KeyboardInput::WaitForKey()
{
	while (!PollKey()) {
	}

	m_keyPressed = 1;
	if (m_key == c_keyEscape) {
		return 3;
	}
	else {
		return 1;
	}
}

undefined4 KeyboardInput::GetKeyPressed()
{
	return m_keyPressed;
}

// Returns 0 without a key, 3 for Escape and 1 for any other key.
MechS32 KeyboardInput::PollKey()
{
	m_key = KeyboardPollKeyCode();
	if (m_key != 0) {
		m_keyPressed = 1;
		m_key &= ~c_keyCodeControl;
		if (m_key & c_keyCodeShift) {
			m_key &= ~c_keyCodeShift;
			m_key = toupper(m_key);
		}

		if (m_key == c_keyEscape) {
			return 3;
		}
		else {
			return 1;
		}
	}
	else {
		m_keyPressed = 0;
		return 0;
	}
}

void KeyboardInput::ResetText(undefined4 p_maxLength)
{
	m_key = 0;
	m_length = 0;
	m_maxLength = p_maxLength;
	strcpy(m_text, "");
}

void KeyboardInput::SetText(undefined4 p_maxLength, const MechChar* p_text)
{
	strcpy(m_text, p_text);
	m_key = 0;
	m_length = strlen(p_text);
	m_maxLength = p_maxLength;
}

// Returns 1 after an edit, 2 when Return copies the text out, 3 for Escape and 0 otherwise.
MechS32 KeyboardInput::EditText(MechChar* p_text, MechU8 p_upperCase)
{
	if (PollKey() == 1) {
		switch (m_key) {
		case c_keyBackspace:
			m_length--;
			if (m_length < 0) {
				m_length = 0;
			}

			m_text[m_length] = '\0';
			return 1;
		case c_keyReturn:
			m_text[m_length] = '\0';
			strcpy(p_text, m_text);
			return 2;
		case c_keyEscape:
			return 3;
		default:
			if (m_maxLength - 1 == m_length) {
				return 0;
			}

			if (m_key & c_keyCodeAlt) {
				return 0;
			}

			m_text[m_length] = m_key;
			m_length++;
			m_text[m_length] = '\0';
			if (p_upperCase == TRUE) {
				UppercaseString(m_text);
			}

			return 1;
		}
	}

	return 0;
}

// Edits p_text with a cursor ('_') drawn after it. Return, a click or the window closing store
// the text and return 1; Escape stores it and returns 0.
MechS32 EditTextField(
	Font* p_font,
	MechS32 p_left,
	MechS32 p_top,
	MechChar* p_text,
	undefined* p_colors,
	MechS32 p_maxLength,
	MechS32 p_width
)
{
	MechS32 key;
	MechS32 length;
	TextGlyph* glyph;

	glyph = NULL;
	length = strlen(p_text);
	strcpy(g_editTextBuffer, p_text);
	strcat(g_editTextBuffer, "_");
	glyph = p_font->AddOverlayText(p_left, p_top, g_editTextBuffer, p_colors);

	for (;;) {
		UpdateVideos();
		g_videoDriver->DrawShell();
		g_mouseState->ReadMouseState();

		if (!PumpMessage() || g_menuDialogOpen || g_mouseState->GetLeftPressed() == 1) {
			g_editTextBuffer[length] = '\0';
			if (glyph) {
				delete glyph;
			}

			strcpy(p_text, g_editTextBuffer);
			return 1;
		}

		if (g_keyboardInput->PollKey()) {
			switch (g_keyboardInput->m_key) {
			case c_keyBackspace:
				if (length == 0) {
					break;
				}

				length--;
				g_editTextBuffer[length] = '_';
				g_editTextBuffer[length + 1] = '\0';
				if (glyph) {
					delete glyph;
				}

				glyph = p_font->AddOverlayText(p_left, p_top, g_editTextBuffer, p_colors);
				break;
			case c_keyReturn:
				g_editTextBuffer[length] = '\0';
				if (glyph) {
					delete glyph;
				}

				strcpy(p_text, g_editTextBuffer);
				return 1;
			case c_keyEscape:
				g_editTextBuffer[length] = '\0';
				if (glyph) {
					delete glyph;
				}

				strcpy(p_text, g_editTextBuffer);
				return 0;
			default:
				key = g_keyboardInput->m_key;
				if (key < 0x20 || key > 0x7f || key == c_keyTilde) {
					break;
				}

				if (p_maxLength == length) {
					break;
				}

				if (!p_font->GetCharacterWidth(key)) {
					break;
				}

				g_editTextBuffer[length] = key;
				length++;
				g_editTextBuffer[length] = '_';
				g_editTextBuffer[length + 1] = '\0';

				if (p_font->GetTextWidth(g_editTextBuffer) < p_width) {
					if (glyph) {
						delete glyph;
					}

					glyph = p_font->AddOverlayText(p_left, p_top, g_editTextBuffer, p_colors);
				}
				else {
					length--;
					g_editTextBuffer[length] = '_';
					g_editTextBuffer[length + 1] = '\0';
				}
				break;
			}
		}
	}
}
