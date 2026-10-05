#ifndef PAGE_H
#define PAGE_H

#include "collection.h"
#include "decomp.h"
#include "types.h"

class AudioSample;
class Font;
class PopupPicture;
class VideoDriver;

// A page of text laid out from a string with escape codes: \N ends a line, \C centers the next
// one, \T moves to the next tab stop, \S ends the page, \Ann makes the next word a link with id
// nn, and \Bnnn (back nnn pixels) and \Gnnn (to nnn pixels from the left) pass through to the
// glyphs. The lines become TextGlyph items that
// type themselves out to a looping sound.
// SIZE 0x34
class Page {
public:
	enum {
		c_tokenEnd = 0,
		c_tokenNewLine = 1,
		c_tokenCenter = 2,
		c_tokenPageBreak = 3,
		c_tokenWord = 4,
		c_tokenTab = 5,
		c_tokenLink = 6,
		c_tokenBackUp = 7,
		c_tokenGoTo = 8
	};

	// SIZE 0x14
	// The area of a link word and its id.
	struct Link {
		MechS32 m_left;   // 0x00
		MechS32 m_top;    // 0x04
		MechS32 m_right;  // 0x08
		MechS32 m_bottom; // 0x0c
		MechS32 m_id;     // 0x10
	};

	Page(
		Font* p_font,
		VideoDriver* p_videoDriver,
		undefined* p_colors,
		MechS32 p_left,
		MechS32 p_top,
		MechS32 p_width,
		MechS32 p_height
	);
	~Page();

	MechS32 SkipSpaces(MechChar* p_text, MechS32* p_pos, MechS32 p_end);
	MechS32 ReadToken(
		MechChar* p_text,
		MechChar* p_word,
		MechS32* p_pos,
		MechS32 p_end,
		MechS32* p_link,
		MechS32* p_kern
	);
	MechS32 FlushLine(MechChar* p_line, MechChar* p_word);
	MechS32 AppendWord(MechChar* p_line, MechChar* p_word, MechS32 p_width);
	MechChar* Layout(MechChar* p_text);
	void AddGlyph(MechChar* p_text, MechS32 p_left, MechS32 p_top);
	void Show();
	void Restart();
	void TypeStep();
	void Hide();
	void SetBanner(undefined* p_data, MechS32 p_size);

private:
	Collection* m_glyphs;       // 0x00
	undefined* m_colors;        // 0x04
	Font* m_font;               // 0x08
	MechS32 m_left;             // 0x0c
	MechS32 m_top;              // 0x10
	MechS32 m_width;            // 0x14
	MechS32 m_bottom;           // 0x18
	MechS32 m_lineHeight;       // 0x1c
	PopupPicture* m_banner;     // 0x20
	VideoDriver* m_videoDriver; // 0x24
	AudioSample* m_sample;      // 0x28
	undefined4 m_unk0x2c;       // 0x2c — never accessed

public:
	// The archive reader turns the links into buttons.
	Collection* m_links; // 0x30
};

// The globals of page.cpp that other units use.
extern MechU8 g_pageLinkRead;
extern MechU8 g_pageBackUpPending;
extern MechU8 g_pageGoToPending;
extern MechChar g_pageWord[0x400];
extern MechChar g_pageLine[0x400];
extern MechChar g_pageTemp[0x400];

#endif // PAGE_H
