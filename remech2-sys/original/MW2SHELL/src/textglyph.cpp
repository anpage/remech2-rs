#include "textglyph.h"

#include "font.h"
#include "menudata.h"
#include "stringutil.h"
#include "textglyph.h"
#include "types.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdlib.h>
#include <string.h>

// Color remap tables for Font text. The first colors a link word, from its "\A" escape to the
// next space.
uint8_t g_linkColorMap[256] = {0xff, 1};

void InitTextColorMaps()
{
	int32_t i;

	g_linkColorMap[0] = 0xff;
	for (i = 1; i < 256; i++) {
		g_linkColorMap[i] = i;
	}

	g_linkColorMap[1] = 1;
}

void TextGlyph::SetTyped(uint32_t p_typed)
{
	m_typed = p_typed;
}

void TextGlyph::Draw()
{
	if (m_text != NULL && strlen(m_text) != 0) {
		if (m_typed == 1) {
			DrawAllChars();
		}
		else {
			m_font->DrawString(m_left, m_top, m_text, m_colors);
			m_done = 1;
		}
	}
}

void TextGlyph::DrawAllChars()
{
	int32_t textIndex;

	textIndex = m_textIndex;
	m_textIndex = -1;
	while (m_textIndex < textIndex) {
		DrawNextChar();
	}
}

// Draws the next character of the text, or handles the escape sequence starting at it.
uint8_t TextGlyph::DrawNextChar()
{
	int32_t i;
	char number[0x80];

	if (!m_registered) {
		m_videoDriver->AddGlyph(this, 1);
		m_registered = 1;
	}

	if (m_textIndex < 0) {
		m_currentColors = m_colors;
		m_textIndex = 0;
		m_cursorX = m_left;
	}

	if (strlen(m_text) <= m_textIndex) {
		m_currentColors = m_colors;
		m_done = 1;
		return TRUE;
	}

	if (m_text[m_textIndex] == '\\') {
		m_currentColors = m_colors;
		m_textIndex++;

		if (strlen(m_text) > m_textIndex) {
			switch (m_text[m_textIndex]) {
			case 'T':
			case 't':
				m_textIndex++;
				for (i = 0; i < 19; i++) {
					if (m_cursorX - m_left < g_textTabStops[i]) {
						m_cursorX = g_textTabStops[i] + m_left;
						break;
					}
				}
				break;
			case 'B':
			case 'b':
				m_textIndex++;
				number[0] = m_text[m_textIndex];
				m_textIndex++;
				number[1] = m_text[m_textIndex];
				m_textIndex++;
				number[2] = m_text[m_textIndex];
				m_textIndex++;
				number[3] = '\0';
				m_cursorX -= atoi(number);
				break;
			case 'G':
			case 'g':
				m_textIndex++;
				number[0] = m_text[m_textIndex];
				m_textIndex++;
				number[1] = m_text[m_textIndex];
				m_textIndex++;
				number[2] = m_text[m_textIndex];
				m_textIndex++;
				number[3] = '\0';
				m_cursorX = m_left + atoi(number);
				break;
			case 'A':
			case 'a':
				m_textIndex++;
				m_currentColors = g_linkColorMap;
				break;
			default:
				break;
			}
		}
	}
	else {
		if (m_text[m_textIndex] == ' ') {
			m_currentColors = m_colors;
		}

		m_cursorX += m_font->DrawChar(m_cursorX, m_top, m_text[m_textIndex], m_currentColors);
		m_textIndex++;
	}

	if (m_cursorX - m_left + 1 > m_width) {
		m_width = m_cursorX - m_left + 1;
	}

	return FALSE;
}

uint8_t TextGlyph::TypeStep()
{
	if (m_done == 1) {
		return TRUE;
	}

	if (strlen(m_text) == 0) {
		return TRUE;
	}

	return DrawNextChar();
}

TextGlyph::TextGlyph(char* p_text, int32_t p_left, int32_t p_top, uint8_t* p_colors, Font* p_font)
{
	m_font = p_font;
	m_videoDriver = p_font->m_videoDriver;

	if (p_text == NULL) {
		p_text = "";
	}

	// A leading '~' centers the text on p_left.
	if (*p_text == '~') {
		m_text = AllocateString(p_text + 1);
	}
	else {
		m_text = AllocateString(p_text);
	}

	m_width = m_font->GetTextWidth(m_text);
	m_height = m_font->m_height;

	if (*p_text == '~') {
		m_left = p_left - m_width / 2;
	}
	else {
		m_left = p_left;
	}

	m_top = p_top;
	m_right = m_left + m_width;
	m_bottom = m_top + m_height;
	m_done = 0;
	m_cursorX = -1;
	m_textIndex = -1;
	m_typed = 0;
	m_registered = 0;
	m_colors = p_colors;
	m_currentColors = p_colors;
}

// Restores the background under the text and rewinds it.
void TextGlyph::Shutdown()
{
	m_done = 0;
	m_cursorX = -1;
	m_textIndex = -1;
	m_videoDriver->RemoveGlyph(this);
	m_registered = 0;
	m_videoDriver->RestoreBackground(m_left, m_top, m_width, m_height);
}

TextGlyph::~TextGlyph()
{
	Shutdown();

	if (m_text != NULL) {
		MechHeapFree(g_primaryHeap, m_text);
	}
}
