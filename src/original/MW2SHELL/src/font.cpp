#include "font.h"

#include "textglyph.h"
#include "vfxa.h"
#include "videodriver.h"
#include "windowstate.h"

DECOMP_SIZE_ASSERT(Font, 0x414)

// FUNCTION: MW2SHELL 0x10005340
Font::Font(void* p_data, VideoDriver* p_videoDriver)
{
	m_videoDriver = p_videoDriver;
	m_data = p_data;
	m_dataCopy = m_data;
	m_height = VFX_font_height(m_data);
}

// Nothing in the shell deletes a Font, so the destructor has no callers.
// FUNCTION: MW2SHELL 0x10005394
Font::~Font()
{
	HeapFree(g_primaryHeap, HEAP_NO_SERIALIZE, m_data);
}

// FUNCTION: MW2SHELL 0x100053be
MechS32 Font::GetTextWidth(MechChar* p_text)
{
	MechS32 width;

	if (p_text == NULL) {
		return 0;
	}

	width = 0;
	for (; *p_text != '\0'; p_text++) {
		width += VFX_character_width(m_data, *p_text);
	}

	return width;
}

// FUNCTION: MW2SHELL 0x10005424
MechS32 Font::GetCharacterWidth(MechS32 p_char)
{
	return VFX_character_width(m_data, p_char);
}

// Draws p_text and keeps its glyph among the glyphs drawn under the videos.
// FUNCTION: MW2SHELL 0x1000544e
TextGlyph* Font::AddText(MechS32 p_left, MechS32 p_top, MechChar* p_text, undefined* p_colors)
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
// FUNCTION: MW2SHELL 0x10005522
TextGlyph* Font::AddOverlayText(MechS32 p_left, MechS32 p_top, MechChar* p_text, undefined* p_colors)
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
// FUNCTION: MW2SHELL 0x100055f6
TextGlyph* Font::AddTypedText(MechS32 p_left, MechS32 p_top, MechChar* p_text, undefined* p_colors)
{
	TextGlyph* glyph;

	glyph = new TextGlyph(p_text, p_left, p_top, p_colors, this);
	m_videoDriver->AddGlyph(glyph, 0);
	glyph->TypeStep();
	return glyph;
}

// FUNCTION: MW2SHELL 0x100056b9
MechS32 Font::DrawString(MechS32 p_left, MechS32 p_top, MechChar* p_text, undefined* p_colors)
{
	return m_videoDriver->DrawString(p_left, p_top, m_data, p_text, p_colors);
}

// FUNCTION: MW2SHELL 0x100056f5
MechS32 Font::DrawChar(MechS32 p_left, MechS32 p_top, MechS32 p_char, undefined* p_colors)
{
	return m_videoDriver->DrawChar(p_left, p_top, m_data, p_char, p_colors);
}

// Text entry: m_typedRight holds the right edge of each typed character, m_typedCount their count.
// FUNCTION: MW2SHELL 0x10005731
void Font::TypeKey(MechS32 p_left, MechS32 p_top, MechS32 p_key, undefined* p_colors)
{
	MechS32 width;
	MechS32 x;
	MechS32 end;

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

// FUNCTION: MW2SHELL 0x10005913
void Font::ResetTyping()
{
	MechS32 i;

	m_typedCount = 0;
	for (i = 0; i < 0x100; i++) {
		m_typedRight[i] = 0;
	}
}
