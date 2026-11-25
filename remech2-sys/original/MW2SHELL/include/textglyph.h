#ifndef TEXTGLYPH_H
#define TEXTGLYPH_H

#include "types.h"

class Font;
class VideoDriver;

// A line of text drawn with a Font, either at once or one character per step.
#pragma pack(1)
class TextGlyph {
public:
	TextGlyph(char* p_text, int32_t p_left, int32_t p_top, uint8_t* p_colors, Font* p_font);
	~TextGlyph();

	void SetTyped(uint32_t p_typed);
	void Draw();
	void DrawAllChars();
	uint8_t DrawNextChar();
	uint8_t TypeStep();
	void Shutdown();

private:
	Font* m_font;
	uint8_t* m_colors;
	uint8_t* m_currentColors; // m_colors, or the "\A" escape's map
	VideoDriver* m_videoDriver;
	uint32_t m_typed;     // 1: drawn one character per TypeStep
	uint8_t m_registered; // in the video driver's typed glyph list
	char* m_text;

public:
	int32_t m_height;
	int32_t m_width;

private:
	int32_t m_left;
	int32_t m_top;

public:
	// The cockpit controls screen places a field after the previous one's glyph.
	int32_t m_right;

private:
	int32_t m_bottom;

public:
	// Page restarts the typing and checks for the end directly.
	uint8_t m_done; // every character drawn
	int32_t m_cursorX;
	int32_t m_textIndex;
};
#pragma pack()

// The functions and globals of textglyph.cpp that other units use.
extern uint8_t g_linkColorMap[256];
void InitTextColorMaps();

#endif // TEXTGLYPH_H
