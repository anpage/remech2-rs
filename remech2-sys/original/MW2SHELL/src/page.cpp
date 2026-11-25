#include "page.h"

#include "audiosample.h"
#include "elapsed.h"
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

// Set by a \A escape and never read.
uint8_t g_pageLinkRead = false;

// A \B or \G escape was read: the next ReadToken passes it on to the glyph as a word.
uint8_t g_pageBackUpPending = false;

uint8_t g_pageGoToPending = false;

// Layout's word, line and scratch buffers.
char g_pageWord[0x400];

char g_pageLine[0x400];

char g_pageTemp[0x400];

#define c_typeCharsPerSecond 1200

uint32_t g_typeLastTime;
uint32_t g_typeOwed;

// Records the area of the link word just placed. Layout does this in three places.
#define PAGE_ADD_LINK(WORD_WIDTH)                                                                                      \
	if (link != -1) {                                                                                                  \
		rect = (Link*) MechHeapAlloc(g_primaryHeap, sizeof(Link));                                                     \
		if (wrapped == true) {                                                                                         \
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

Page::Page(
	Font* p_font,
	VideoDriver* p_videoDriver,
	uint8_t* p_colors,
	int32_t p_left,
	int32_t p_top,
	int32_t p_width,
	int32_t p_height
)
{
	Collection* glyphs;
	int32_t result;
	void* data;
	int32_t size;

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
int32_t Page::SkipSpaces(char* p_text, int32_t* p_pos, int32_t p_end)
{
	int32_t count;

	count = 0;
	while (*p_pos < p_end && p_text[*p_pos] == ' ') {
		(*p_pos)++;
		count++;
	}

	return count;
}

// Reads the next word or escape code at *p_pos into p_word and returns its c_token kind.
int32_t Page::ReadToken(char* p_text, char* p_word, int32_t* p_pos, int32_t p_end, int32_t* p_link, int32_t* p_offset)
{
	int32_t count;
	char number[5];
	int32_t i;
	int32_t j;

	if (*p_pos >= p_end) {
		return c_tokenEnd;
	}

	if (g_pageBackUpPending == true) {
		g_pageBackUpPending = false;
		sprintf(p_word, "\\b%03d", *p_offset);
		return c_tokenWord;
	}

	if (g_pageGoToPending == true) {
		g_pageGoToPending = false;
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
			g_pageLinkRead = true;
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
			g_pageBackUpPending = true;
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
			g_pageGoToPending = true;
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
int32_t Page::FlushLine(char* p_line, char* p_word)
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
int32_t Page::AppendWord(char* p_line, char* p_word, int32_t p_width)
{
	strcat(p_line, p_word);
	if (p_width != -1) {
		return p_width;
	}

	return m_font->GetTextWidth(p_word);
}

// Lays out p_text until the page is full or a \S code. Returns where the next page starts, or
// NULL at the end of the text.
char* Page::Layout(char* p_text)
{
	int32_t offset;
	int32_t link;
	int32_t gap;
	int32_t count;
	int32_t i;
	int32_t tabs;
	int32_t lineWidth;
	uint8_t wrapped;
	int32_t pos;
	int32_t token;
	int32_t length;
	char* wordStart;
	int32_t stop;
	int32_t width;
	Link* rect;
	int32_t half;
	uint8_t center;

	rect = NULL;
	length = strlen(p_text);
	pos = 0;
	lineWidth = 0;
	strcpy(g_pageLine, "");
	center = false;
	tabs = 0;
	link = -1;
	offset = -1;
	token = c_tokenWord;

	while (token != c_tokenEnd) {
		wordStart = p_text + pos;
		token = ReadToken(p_text, g_pageWord, &pos, length, &link, &offset);
		wrapped = false;

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
				wrapped = false;
			}
			else {
				lineWidth = FlushLine(g_pageLine, g_pageWord);
				wrapped = true;
			}

			PAGE_ADD_LINK(m_font->GetTextWidth(g_pageWord));

			if (m_bottom > 0 && m_lineHeight + m_top > m_bottom) {
				return wordStart;
			}
			break;
		case c_tokenCenter:
			center = true;
			break;
		case c_tokenNewLine:
			width = m_font->GetTextWidth(g_pageWord);
			if (center == true) {
				center = false;
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

void Page::AddGlyph(char* p_text, int32_t p_left, int32_t p_top)
{
	TextGlyph* glyph;

	glyph = new TextGlyph(p_text, p_left, p_top, m_colors, m_font);
	ExpandCollection(m_glyphs, glyph);
}

void Page::Show()
{
	TextGlyph* glyph;
	int32_t i;

	if (m_banner) {
		m_banner->Show();
	}

	for (i = 0; i < m_glyphs->m_count; i++) {
		glyph = (TextGlyph*) CollectionGet(m_glyphs, i);
		glyph->Draw();
	}
}

// Starts typing the page out again.
void Page::Restart()
{
	TextGlyph* glyph;
	int32_t i;

	if (m_banner) {
		m_banner->Show();
	}

	if (m_sample && !m_sample->IsPlaying()) {
		m_sample->Start();
	}

	for (i = 0; i < m_glyphs->m_count; i++) {
		glyph = (TextGlyph*) CollectionGet(m_glyphs, i);
		glyph->m_done = false;
		glyph->m_cursorX = -1;
		glyph->m_textIndex = -1;
		glyph->SetTyped(1);
	}

	g_typeLastTime = MechMilliseconds();
	g_typeOwed = 0;
}

// Advances the glyphs still typing and stops the sound when done.
// The original advanced one character per call.
void Page::TypeStep()
{
	TextGlyph* glyph;
	int32_t i;
	uint32_t now;
	uint32_t steps;

	now = MechMilliseconds();
	g_typeOwed += (now - g_typeLastTime) * c_typeCharsPerSecond;
	g_typeLastTime = now;
	steps = g_typeOwed / 1000;
	g_typeOwed %= 1000;

	for (i = 0; i < m_glyphs->m_count; i++) {
		glyph = (TextGlyph*) CollectionGet(m_glyphs, i);
		while (!glyph->m_done) {
			if (!steps) {
				return;
			}
			glyph->TypeStep();
			steps--;
		}
	}

	if (m_sample) {
		m_sample->Stop();
	}
}

void Page::Hide()
{
	TextGlyph* glyph;
	int32_t i;

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

void Page::SetBanner(uint8_t* p_data, int32_t p_size)
{
	m_banner = new PopupPicture(p_data, p_size, m_videoDriver, 0, 0, m_sample);
}

Page::~Page()
{
	TextGlyph* glyph;
	int32_t i;

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
