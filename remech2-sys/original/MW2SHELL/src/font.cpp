#include "font.h"

#include "textglyph.h"
#include "vfxa.h"
#include "videodriver.h"
#include "windowstate.h"

Font::Font(void* p_data, VideoDriver* p_videoDriver)
{
	m_videoDriver = p_videoDriver;
	m_data = p_data;
	m_dataCopy = m_data;
	m_height = VFX_font_height(m_data);
}

// Nothing in the shell deletes a Font, so the destructor has no callers.
Font::~Font()
{
	MechHeapFree(g_primaryHeap, m_data);
}

int32_t Font::GetTextWidth(char* p_text)
{
	int32_t width;

	if (p_text == NULL) {
		return 0;
	}

	width = 0;
	for (; *p_text != '\0'; p_text++) {
		width += VFX_character_width(m_data, *p_text);
	}

	return width;
}

int32_t Font::GetCharacterWidth(int32_t p_char)
{
	return VFX_character_width(m_data, p_char);
}

// Draws p_text and keeps its glyph among the glyphs drawn under the videos.
TextGlyph* Font::AddText(int32_t p_left, int32_t p_top, char* p_text, undefined* p_colors)
{
	TextGlyph* glyph;

	if (p_text == NULL) {
		p_text = "";
	}

	glyph = new TextGlyph(p_text, p_left, p_top, p_colors, this);
	m_videoDriver->AddGlyph(glyph, 0);
	glyph->Draw();
	return glyph;
}

// Draws p_text and keeps its glyph among the glyphs drawn over the videos.
TextGlyph* Font::AddOverlayText(int32_t p_left, int32_t p_top, char* p_text, undefined* p_colors)
{
	TextGlyph* glyph;

	if (p_text == NULL) {
		p_text = "";
	}

	glyph = new TextGlyph(p_text, p_left, p_top, p_colors, this);
	m_videoDriver->AddGlyph(glyph, 1);
	glyph->Draw();
	return glyph;
}

// Like AddText, but starts typing the text out (TextGlyph::TypeStep) instead of drawing it whole.
TextGlyph* Font::AddTypedText(int32_t p_left, int32_t p_top, char* p_text, undefined* p_colors)
{
	TextGlyph* glyph;

	glyph = new TextGlyph(p_text, p_left, p_top, p_colors, this);
	m_videoDriver->AddGlyph(glyph, 0);
	glyph->TypeStep();
	return glyph;
}

int32_t Font::DrawString(int32_t p_left, int32_t p_top, char* p_text, undefined* p_colors)
{
	return m_videoDriver->DrawString(p_left, p_top, m_data, p_text, p_colors);
}

int32_t Font::DrawChar(int32_t p_left, int32_t p_top, int32_t p_char, undefined* p_colors)
{
	return m_videoDriver->DrawChar(p_left, p_top, m_data, p_char, p_colors);
}

// Text entry: m_typedRight holds the right edge of each typed character, m_typedCount their count.
void Font::TypeKey(int32_t p_left, int32_t p_top, int32_t p_key, undefined* p_colors)
{
	int32_t width;
	int32_t x;
	int32_t end;

	switch (p_key) {
	case '\b':
		m_typedCount--;
		if (m_typedCount < 0) {
			m_typedCount = 0;
		}

		if (m_typedCount == 0) {
			if (m_typedRight[m_typedCount] == 0) {
				return;
			}

			x = p_left;
			end = m_typedRight[m_typedCount] + p_left;
		}
		else {
			x = m_typedRight[m_typedCount - 1] + p_left;
			end = m_typedRight[m_typedCount] + p_left;
		}

		m_typedRight[m_typedCount] = 0;
		while (x < end) {
			x += DrawChar(x, p_top, ' ', p_colors);
		}
		break;
	case '\r':
		break;
	case '\x1b':
		ResetTyping();
		break;
	default:
		if (m_typedCount == 0) {
			x = 0;
		}
		else {
			x = m_typedRight[m_typedCount - 1];
		}

		width = DrawChar(x + p_left, p_top, p_key, p_colors);
		if (m_typedCount > 0) {
			m_typedRight[m_typedCount] = m_typedRight[m_typedCount - 1] + width;
		}
		else {
			m_typedRight[m_typedCount] = width;
		}

		m_typedCount++;
		break;
	}
}

void Font::ResetTyping()
{
	int32_t i;

	m_typedCount = 0;
	for (i = 0; i < 0x100; i++) {
		m_typedRight[i] = 0;
	}
}
