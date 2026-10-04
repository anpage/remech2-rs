#include "page.h"

#include "audiosample.h"
#include "font.h"
#include "menudata.h"
#include "popuppicture.h"
#include "shellglobals.h"
#include "textglyph.h"
#include "tmpackdatabase.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

DECOMP_SIZE_ASSERT(Page, 0x34)
DECOMP_SIZE_ASSERT(Page::Link, 0x14)

// Set by a \A escape and never read.
// GLOBAL: MW2SHELL 0x10071180
MechU8 g_pageLinkRead = FALSE;

// A \B or \G escape was read: the next ReadToken passes it on to the glyph as a word.
// GLOBAL: MW2SHELL 0x10071184
MechU8 g_pageBackUpPending = FALSE;

// GLOBAL: MW2SHELL 0x10071188
MechU8 g_pageGoToPending = FALSE;

// Layout's word, line and scratch buffers.
// GLOBAL: MW2SHELL 0x10093a78
MechChar g_pageWord[0x400];

// GLOBAL: MW2SHELL 0x10093e78
MechChar g_pageLine[0x400];

// GLOBAL: MW2SHELL 0x10094278
MechChar g_pageTemp[0x400];

// Records the area of the link word just placed. Layout does this in three places.
#define PAGE_ADD_LINK(WORD_WIDTH)                                                                                      \
	if (link != -1) {                                                                                                  \
		rect = (Link*) MechHeapAlloc(g_primaryHeap, sizeof(Link));                                      \
		if (wrapped == TRUE) {                                                                                         \
			rect->m_left = m_left;                                                                                     \
			rect->m_top = m_top;                                                                                       \
			rect->m_right = rect->m_left + (WORD_WIDTH);                                                               \
			rect->m_bottom = rect->m_top + m_lineHeight;                                                               \
		}                                                                                                              \
		else {                                                                                                         \
			rect->m_left = m_left + lineWidth - (WORD_WIDTH);                                                          \
			rect->m_top = m_font->m_height + m_top;                                                                    \
			rect->m_right = rect->m_left + (WORD_WIDTH);                                                               \
			rect->m_bottom = rect->m_top + m_lineHeight;                                                               \
		}                                                                                                              \
                                                                                                                       \
		rect->m_id = link;                                                                                             \
		ExpandCollection(m_links, rect);                                                                               \
		link = -1;                                                                                                     \
	}

// Stack-slot permutation: glyphs, result, data and size.
// FUNCTION: MW2SHELL 0x10044880
Page::Page(
	Font* p_font,
	VideoDriver* p_videoDriver,
	undefined* p_colors,
	MechS32 p_left,
	MechS32 p_top,
	MechS32 p_width,
	MechS32 p_height
)
{
	Collection* glyphs;
	MechS32 result;
	void* data;
	MechS32 size;

	m_font = p_font;
	m_videoDriver = p_videoDriver;
	m_left = p_left;
	m_top = p_top;
	m_width = p_width;

	g_mw2Database->GetDBItem(0x4d, &data, &size);
	m_sample = new AudioSample(g_audioSubsystem, data, size);
	m_sample->EnableLoop();
	CreateCollection(&m_links, 10, NULL, 4, NULL);

	if (p_height < 0) {
		p_height = 100000;
	}
	else {
		m_bottom = m_top + p_height;
	}

	m_banner = NULL;
	m_colors = p_colors;
	m_lineHeight = m_font->m_height + 2;
	m_top -= m_lineHeight;

	result = CreateCollection(&glyphs, 0x32, NULL, 4, NULL);
	if (result) {
		fprintf(stderr, "Could not create collection for Page object\n");
		fflush(stderr);
		exit(1);
	}

	m_glyphs = glyphs;
}

// Returns the number of spaces skipped.
// FUNCTION: MW2SHELL 0x10044a44
MechS32 Page::SkipSpaces(MechChar* p_text, MechS32* p_pos, MechS32 p_end)
{
	MechS32 count;

	count = 0;
	while (*p_pos < p_end && p_text[*p_pos] == ' ') {
		(*p_pos)++;
		count++;
	}

	return count;
}

// Reads the next word or escape code at *p_pos into p_word and returns its c_token kind.
// FUNCTION: MW2SHELL 0x10044a96
MechS32 Page::ReadToken(
	MechChar* p_text,
	MechChar* p_word,
	MechS32* p_pos,
	MechS32 p_end,
	MechS32* p_link,
	MechS32* p_offset
)
{
	MechS32 count;
	MechChar number[5];
	MechS32 i;
	MechS32 j;

	if (*p_pos >= p_end) {
		return c_tokenEnd;
	}

	if (g_pageBackUpPending == TRUE) {
		g_pageBackUpPending = FALSE;
		sprintf(p_word, "\\b%03d", *p_offset);
		return c_tokenWord;
	}

	if (g_pageGoToPending == TRUE) {
		g_pageGoToPending = FALSE;
		sprintf(p_word, "\\g%03d", *p_offset);
		return c_tokenWord;
	}

	strcpy(p_word, "");
	j = 0;
	i = *p_pos;

	// An unknown escape code is kept in the word, which goes on from here.
nextChar:
	while (i < p_end && p_text[i] != ' ' && p_text[i] != '\\') {
		p_word[j] = p_text[i] & 0x7f;
		i++;
		j++;
	}

	if (i >= p_end) {
		p_word[j] = '\0';
		*p_pos = i;
		return c_tokenEnd;
	}

	if ((p_text[i] == '\\' || p_text[i] == ' ') && j > 0) {
		p_word[j] = '\0';
		*p_pos = i;
		count = SkipSpaces(p_text, p_pos, p_end);
		for (i = 0; i < count; i++) {
			strcat(p_word, " ");
		}

		return c_tokenWord;
	}

	if (p_text[i] == '\\') {
		switch (p_text[++i]) {
		case 'S':
		case 's':
			*p_pos = ++i;
			SkipSpaces(p_text, p_pos, p_end);
			return c_tokenPageBreak;
		case 'N':
		case 'n':
			*p_pos = ++i;
			SkipSpaces(p_text, p_pos, p_end);
			return c_tokenNewLine;
		case 'C':
		case 'c':
			*p_pos = ++i;
			SkipSpaces(p_text, p_pos, p_end);
			return c_tokenCenter;
		case 'T':
		case 't':
			*p_pos = ++i;
			SkipSpaces(p_text, p_pos, p_end);
			return c_tokenTab;
		case 'A':
		case 'a':
			number[0] = p_text[++i];
			number[1] = p_text[++i];
			i++;
			number[2] = '\0';
			g_pageLinkRead = TRUE;
			*p_link = atoi(number);
			*p_pos = i;
			SkipSpaces(p_text, p_pos, p_end);
			return c_tokenLink;
		case 'B':
		case 'b':
			number[0] = p_text[++i];
			number[1] = p_text[++i];
			number[2] = p_text[++i];
			i++;
			number[3] = '\0';
			g_pageBackUpPending = TRUE;
			*p_offset = atoi(number);
			*p_pos = i;
			SkipSpaces(p_text, p_pos, p_end);
			return c_tokenBackUp;
		case 'G':
		case 'g':
			number[0] = p_text[++i];
			number[1] = p_text[++i];
			number[2] = p_text[++i];
			i++;
			number[3] = '\0';
			g_pageGoToPending = TRUE;
			*p_offset = atoi(number);
			*p_pos = i;
			SkipSpaces(p_text, p_pos, p_end);
			return c_tokenGoTo;
		default:
			i++;
			p_word[j] = '\\';
			j++;
			goto nextChar;
		}
	}

	return c_tokenWord;
}

// Moves down a line and turns p_line into a glyph, then starts the next line with p_word.
// Returns the width of the new line.
// FUNCTION: MW2SHELL 0x10044f3b
MechS32 Page::FlushLine(MechChar* p_line, MechChar* p_word)
{
	m_top += m_lineHeight;
	if (strlen(p_line) == 0) {
		return 0;
	}

	AddGlyph(p_line, m_left, m_top);
	strcpy(p_line, "");
	if (p_word == NULL) {
		return 0;
	}

	strcpy(p_line, p_word);
	return m_font->GetTextWidth(p_word);
}

// Returns the width p_word adds, measuring it unless p_width gives it.
// FUNCTION: MW2SHELL 0x10045005
MechS32 Page::AppendWord(MechChar* p_line, MechChar* p_word, MechS32 p_width)
{
	strcat(p_line, p_word);
	if (p_width != -1) {
		return p_width;
	}

	return m_font->GetTextWidth(p_word);
}

// Lays out p_text until the page is full or a \S code. Returns where the next page starts, or
// NULL at the end of the text.
// Only a stack-slot permutation of the locals remains. Declaring count and i ahead of tabs
// makes the two string loops compare their operands in the original order.
// FUNCTION: MW2SHELL 0x1004506f
MechChar* Page::Layout(MechChar* p_text)
{
	MechS32 offset;
	MechS32 link;
	MechS32 gap;
	MechS32 count;
	MechS32 i;
	MechS32 tabs;
	MechS32 lineWidth;
	MechU8 wrapped;
	MechS32 pos;
	MechS32 token;
	MechS32 length;
	MechChar* wordStart;
	MechS32 stop;
	MechS32 width;
	Link* rect;
	MechS32 half;
	MechU8 center;

	rect = NULL;
	length = strlen(p_text);
	pos = 0;
	lineWidth = 0;
	strcpy(g_pageLine, "");
	center = FALSE;
	tabs = 0;
	link = -1;
	offset = -1;
	token = c_tokenWord;

	while (token != c_tokenEnd) {
		wordStart = p_text + pos;
		token = ReadToken(p_text, g_pageWord, &pos, length, &link, &offset);
		wrapped = FALSE;

		switch (token) {
		case c_tokenWord:
			width = m_font->GetTextWidth(g_pageWord);
			strcpy(g_pageTemp, g_pageWord);
			strcpy(g_pageWord, "");
			for (i = 0; i < tabs; i++) {
				strcat(g_pageWord, "\\t");
			}

			tabs = 0;
			if (link != -1) {
				strcat(g_pageWord, "\\a");
			}

			strcat(g_pageWord, g_pageTemp);
			if (offset != -1) {
				lineWidth -= offset;
				offset = -1;
			}

			if (lineWidth + m_font->GetTextWidth(g_pageWord) <= m_width) {
				lineWidth += AppendWord(g_pageLine, g_pageWord, width);
				wrapped = FALSE;
			}
			else {
				lineWidth = FlushLine(g_pageLine, g_pageWord);
				wrapped = TRUE;
			}

			PAGE_ADD_LINK(m_font->GetTextWidth(g_pageWord));

			if (m_bottom > 0 && m_lineHeight + m_top > m_bottom) {
				return wordStart;
			}
			break;
		case c_tokenCenter:
			center = TRUE;
			break;
		case c_tokenNewLine:
			width = m_font->GetTextWidth(g_pageWord);
			if (center == TRUE) {
				center = FALSE;
				gap = m_width - lineWidth;
				half = gap / 2;
				count = half / m_font->GetTextWidth(" ");
				strcpy(g_pageTemp, g_pageLine);
				strcpy(g_pageLine, "");
				for (i = 0; i < count; i++) {
					strcat(g_pageLine, " ");
				}

				strcat(g_pageLine, g_pageTemp);
			}

			if (m_bottom > 0 && m_lineHeight + m_top > m_bottom) {
				return p_text + pos;
			}

			lineWidth = FlushLine(g_pageLine, NULL);
			PAGE_ADD_LINK(width);
			break;
		case c_tokenPageBreak:
			return p_text + pos;
		case c_tokenLink:
			break;
		case c_tokenBackUp:
		case c_tokenGoTo:
			break;
		case c_tokenTab:
			tabs++;
			for (stop = 0; stop < 19; stop++) {
				if (stop == 18) {
					lineWidth = g_textTabStops[18];
					break;
				}

				if (g_textTabStops[stop] > lineWidth) {
					lineWidth = g_textTabStops[stop];
					break;
				}
			}
			break;
		default:
			strcpy(g_pageWord, "");
			tabs = 0;
			break;
		}
	}

	width = m_font->GetTextWidth(g_pageWord);
	if (strlen(g_pageWord)) {
		lineWidth += AppendWord(g_pageLine, g_pageWord, width);
	}

	FlushLine(g_pageLine, NULL);
	PAGE_ADD_LINK(width);

	return NULL;
}

// FUNCTION: MW2SHELL 0x10045844
void Page::AddGlyph(MechChar* p_text, MechS32 p_left, MechS32 p_top)
{
	TextGlyph* glyph;

	glyph = new TextGlyph(p_text, p_left, p_top, m_colors, m_font);
	ExpandCollection(m_glyphs, glyph);
}

// FUNCTION: MW2SHELL 0x100458ff
void Page::Show()
{
	TextGlyph* glyph;
	MechS32 i;

	if (m_banner) {
		m_banner->Show();
	}

	for (i = 0; i < m_glyphs->m_count; i++) {
		glyph = (TextGlyph*) CollectionGet(m_glyphs, i);
		glyph->Draw();
	}
}

// Starts typing the page out again.
// FUNCTION: MW2SHELL 0x1004596f
void Page::Restart()
{
	TextGlyph* glyph;
	MechS32 i;

	if (m_banner) {
		m_banner->Show();
	}

	if (m_sample && !m_sample->IsPlaying()) {
		m_sample->Start();
	}

	for (i = 0; i < m_glyphs->m_count; i++) {
		glyph = (TextGlyph*) CollectionGet(m_glyphs, i);
		glyph->m_done = FALSE;
		glyph->m_cursorX = -1;
		glyph->m_textIndex = -1;
		glyph->SetTyped(1);
	}
}

// Advances the first glyph that is still typing; once all are done, stops the sound.
// FUNCTION: MW2SHELL 0x10045a2b
void Page::TypeStep()
{
	TextGlyph* glyph;
	MechS32 i;

	for (i = 0; i < m_glyphs->m_count; i++) {
		glyph = (TextGlyph*) CollectionGet(m_glyphs, i);
		if (!glyph->m_done) {
			glyph->TypeStep();
			return;
		}
	}

	if (m_sample) {
		m_sample->Stop();
	}
}

// FUNCTION: MW2SHELL 0x10045ab0
void Page::Hide()
{
	TextGlyph* glyph;
	MechS32 i;

	if (m_banner) {
		m_banner->Hide();
	}

	if (m_sample) {
		m_sample->Stop();
	}

	for (i = 0; i < m_glyphs->m_count; i++) {
		glyph = (TextGlyph*) CollectionGet(m_glyphs, i);
		glyph->Shutdown();
	}
}

// FUNCTION: MW2SHELL 0x10045b38
void Page::SetBanner(undefined* p_data, MechS32 p_size)
{
	m_banner = new PopupPicture(p_data, p_size, m_videoDriver, 0, 0, m_sample);
}

// FUNCTION: MW2SHELL 0x10045be7
Page::~Page()
{
	TextGlyph* glyph;
	MechS32 i;

	if (m_banner) {
		delete m_banner;
	}

	if (m_sample) {
		m_sample->Stop();
		delete m_sample;
	}

	for (i = 0; i < m_glyphs->m_count; i++) {
		glyph = (TextGlyph*) CollectionGet(m_glyphs, i);
		delete glyph;
	}

	MechHeapFree(g_primaryHeap, m_glyphs->m_items);
	MechHeapFree(g_primaryHeap, m_glyphs);
	MechHeapFree(g_primaryHeap, m_links->m_items);
	MechHeapFree(g_primaryHeap, m_links);
}
