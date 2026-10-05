#ifndef TEXTGLYPH_H
#define TEXTGLYPH_H

#include "decomp.h"
#include "types.h"

class Font;
class VideoDriver;

// A line of text drawn with a Font, either at once or one character per step.
// SIZE 0x3e
#pragma pack(1)
class TextGlyph {
public:
	TextGlyph(MechChar* p_text, MechS32 p_left, MechS32 p_top, undefined* p_colors, Font* p_font);
	~TextGlyph();

	void SetTyped(undefined4 p_typed);
	void Draw();
	void DrawAllChars();
	MechU8 DrawNextChar();
	MechU8 TypeStep();
	void Shutdown();

private:
	Font* m_font;               // 0x00
	undefined* m_colors;        // 0x04
	undefined* m_currentColors; // 0x08 — m_colors, or the "\A" escape's map
	VideoDriver* m_videoDriver; // 0x0c
	undefined4 m_typed;         // 0x10 — 1: drawn one character per TypeStep
	MechU8 m_registered;        // 0x14 — in the video driver's typed glyph list
	undefined4 m_unk0x15;       // 0x15 — only cleared, by the constructor
	MechChar* m_text;           // 0x19

public:
	// The shell's field tables read the size directly: an inline accessor would leave a jmp at /Ob1.
	MechS32 m_height; // 0x1d
	MechS32 m_width;  // 0x21

private:
	MechS32 m_left; // 0x25
	MechS32 m_top;  // 0x29

public:
	// The cockpit controls screen places a field after the previous one's glyph.
	MechS32 m_right; // 0x2d

private:
	MechS32 m_bottom; // 0x31

public:
	// Page restarts the typing and checks for the end directly.
	MechU8 m_done;       // 0x35 — every character drawn
	MechS32 m_cursorX;   // 0x36
	MechS32 m_textIndex; // 0x3a
};
#pragma pack()

// The functions and globals of textglyph.cpp that other units use.
extern undefined g_linkColorMap[256];
extern undefined g_unk0x10074758[256];
extern undefined g_unk0x10074858[256];
void InitTextColorMaps();

#endif // TEXTGLYPH_H
