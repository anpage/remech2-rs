#include "textpages.h"

#include "collection.h"
#include "font.h"
#include "page.h"
#include "pilotrecord.h"
#include "projectarchive.h"
#include "shellglobals.h"
#include "stringutil.h"
#include "types.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The index of the HTXT tag in g_bwdTags (projectarchive.cpp).
enum {
	c_tagHtxt = 62
};

// The colors of the pages LoadTextPages lays out.
uint8_t g_textPageColors[0x100];

// The text with its escapes expanded: \\Q the quote, \\R0 to \\R2 the pilot's rank and the next
// two, \\H the pilot's honor.
char g_expandedText[0x2000];

// Expands the escapes of p_text into g_expandedText and returns a copy of the result.
char* ExpandTextEscapes(char* p_text, char* p_quote)
{
	int32_t i;
	int32_t length;
	char number[0x80];
	int32_t textLength;
	int32_t rank;

	textLength = strlen(p_text);
	i = 0;
	length = 0;
	while (i < textLength) {
		if (p_text[i] == '\\') {
			i++;
			switch (p_text[i]) {
			case 'Q':
			case 'q':
				i++;
				g_expandedText[length] = '\0';
				strcat(g_expandedText, p_quote);
				length = strlen(g_expandedText);
				break;
			case 'R':
			case 'r':
				i++;
				g_expandedText[length] = '\0';
				switch (p_text[i]) {
				case '0':
					i++;
					rank = g_currentPilot->m_rank;
					strcat(g_expandedText, g_rankNames[rank]);
					length = strlen(g_expandedText);
					break;
				case '1':
					i++;
					rank = g_currentPilot->m_rank + 1;
					if (rank > 9) {
						rank = 9;
					}
					strcat(g_expandedText, g_rankNames[rank]);
					length = strlen(g_expandedText);
					break;
				case '2':
					i++;
					rank = g_currentPilot->m_rank + 2;
					if (rank > 9) {
						rank = 9;
					}
					strcat(g_expandedText, g_rankNames[rank]);
					length = strlen(g_expandedText);
					break;
				default:
					break;
				}
				break;
			case 'H':
			case 'h':
				i++;
				g_expandedText[length] = '\0';
				sprintf(number, "%d", g_currentPilot->m_honor);
				strcat(g_expandedText, number);
				length = strlen(g_expandedText);
				break;
			default:
				g_expandedText[length] = '\\';
				length++;
				g_expandedText[length] = p_text[i];
				i++;
				length++;
				break;
			}
		}
		else {
			g_expandedText[length] = p_text[i];
			i++;
			length++;
		}
	}

	g_expandedText[length] = '\0';
	return AllocateString(g_expandedText);
}

// Lays out p_text (p_size bytes, the last of which becomes its terminator) on as many pages as
// it takes, adding them to p_pages. With p_quote, the text's escapes are expanded first.
void LayoutTextPages(
	char* p_text,
	int32_t p_size,
	Collection* p_pages,
	Font* p_font,
	uint8_t* p_colors,
	int32_t p_left,
	int32_t p_top,
	int32_t p_width,
	int32_t p_height,
	char* p_quote
)
{
	Page* page;
	char* text;

	p_text[p_size - 1] = '\0';
	if (p_quote) {
		text = ExpandTextEscapes(p_text, p_quote);
	}
	else {
		text = p_text;
	}

	do {
		page = new Page(p_font, g_videoDriver, p_colors, p_left, p_top, p_width, p_height);
		text = page->Layout(text);
		ExpandCollection(p_pages, page);
	} while (text);

	MechHeapFree(g_primaryHeap, text);
}

// Lays out the text (HTXT node) of the project file p_name on pages, in colors that draw 0
// transparent and everything else in color 1.
void LoadTextPages(
	Collection* p_pages,
	int32_t p_left,
	int32_t p_top,
	int32_t p_width,
	int32_t p_height,
	char* p_name,
	Font* p_font,
	char* p_quote
)
{
	int32_t* node = NULL;
	int32_t i;

	g_textPageColors[0] = 0xff;
	g_textPageColors[1] = 1;
	for (i = 2; i < 0x100; i++) {
		g_textPageColors[i] = 0xff;
	}

	if (!g_projectArchive->LoadBwd(p_name)) {
		// The original does nothing about a missing file.
	}

	node = g_projectArchive->FindBwdNode(*(int32_t*) g_bwdTags[c_tagHtxt]);
	if (node) {
		LayoutTextPages(
			(char*) (node + 2),
			node[1] - 8,
			p_pages,
			p_font,
			g_textPageColors,
			p_left,
			p_top,
			p_width,
			p_height,
			p_quote
		);
	}
}
