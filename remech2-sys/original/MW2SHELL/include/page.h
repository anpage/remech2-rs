#ifndef PAGE_H
#define PAGE_H

#include "collection.h"
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

	// The area of a link word and its id.
	struct Link {
		int32_t m_left;   // 0x00
		int32_t m_top;    // 0x04
		int32_t m_right;  // 0x08
		int32_t m_bottom; // 0x0c
		int32_t m_id;     // 0x10
	};

	Page(
		Font* p_font,
		VideoDriver* p_videoDriver,
		uint8_t* p_colors,
		int32_t p_left,
		int32_t p_top,
		int32_t p_width,
		int32_t p_height
	);
	~Page();

	int32_t SkipSpaces(char* p_text, int32_t* p_pos, int32_t p_end);
	int32_t ReadToken(char* p_text, char* p_word, int32_t* p_pos, int32_t p_end, int32_t* p_link, int32_t* p_kern);
	int32_t FlushLine(char* p_line, char* p_word);
	int32_t AppendWord(char* p_line, char* p_word, int32_t p_width);
	char* Layout(char* p_text);
	void AddGlyph(char* p_text, int32_t p_left, int32_t p_top);
	void Show();
	void Restart();
	void TypeStep();
	void Hide();
	void SetBanner(uint8_t* p_data, int32_t p_size);

private:
	Collection* m_glyphs;       // 0x00
	uint8_t* m_colors;          // 0x04
	Font* m_font;               // 0x08
	int32_t m_left;             // 0x0c
	int32_t m_top;              // 0x10
	int32_t m_width;            // 0x14
	int32_t m_bottom;           // 0x18
	int32_t m_lineHeight;       // 0x1c
	PopupPicture* m_banner;     // 0x20
	VideoDriver* m_videoDriver; // 0x24
	AudioSample* m_sample;      // 0x28

public:
	// The archive reader turns the links into buttons.
	Collection* m_links; // 0x30
};

// The globals of page.cpp that other units use.
extern uint8_t g_pageLinkRead;
extern uint8_t g_pageBackUpPending;
extern uint8_t g_pageGoToPending;
extern char g_pageWord[0x400];
extern char g_pageLine[0x400];
extern char g_pageTemp[0x400];

#endif // PAGE_H
