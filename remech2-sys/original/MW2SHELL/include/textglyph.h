#ifndef TEXTGLYPH_H
#define TEXTGLYPH_H

#include "decomp.h"
#include "types.h"

class Font;
class VideoDriver;

// A line of text drawn with a Font, either at once or one character per step.
#pragma pack(1)
class TextGlyph {
public:
	TextGlyph(char* p_text, int32_t p_left, int32_t p_top, undefined* p_colors, Font* p_font);
	~TextGlyph();

	void SetTyped(undefined4 p_typed);
	void Draw();
	void DrawAllChars();
	uint8_t DrawNextChar();
	uint8_t TypeStep();
	void Shutdown();

private:
	Font* m_font;               // 0x00
	undefined* m_colors;        // 0x04
	undefined* m_currentColors; // 0x08 — m_colors, or the "\A" escape's map
	VideoDriver* m_videoDriver; // 0x0c
	undefined4 m_typed;         // 0x10 — 1: drawn one character per TypeStep
	uint8_t m_registered;       // 0x14 — in the video driver's typed glyph list
	char* m_text;               // 0x19

public:
	int32_t m_height; // 0x1d
	int32_t m_width;  // 0x21

private:
	int32_t m_left; // 0x25
	int32_t m_top;  // 0x29

public:
	// The cockpit controls screen places a field after the previous one's glyph.
	int32_t m_right; // 0x2d

private:
	int32_t m_bottom; // 0x31

public:
	// Page restarts the typing and checks for the end directly.
	uint8_t m_done;      // 0x35 — every character drawn
	int32_t m_cursorX;   // 0x36
	int32_t m_textIndex; // 0x3a
};
#pragma pack()

// The functions and globals of textglyph.cpp that other units use.
extern undefined g_linkColorMap[256];
void InitTextColorMaps();

#endif // TEXTGLYPH_H
