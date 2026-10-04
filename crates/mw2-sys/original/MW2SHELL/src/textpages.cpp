#include "textpages.h"

#include "collection.h"
#include "decomp.h"
#include "files.h"
#include "font.h"
#include "page.h"
#include "pilotrecord.h"
#include "projectarchive.h"
#include "shellglobals.h"
#include "stringutil.h"
#include "types.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// The index of the HTXT tag in g_bwdTags (projectarchive.cpp).
enum {
	c_tagHtxt = 62
};

// The colors of the pages LoadTextPages lays out.
// GLOBAL: MW2SHELL 0x1008f658
MechU8 g_textPageColors[0x100];

// The text with its escapes expanded: \\Q the quote, \\R0 to \\R2 the pilot's rank and the next
// two, \\H the pilot's honor. The original also writes the unexpanded text to tmp.out.
// GLOBAL: MW2SHELL 0x1008d658
MechChar g_expandedText[0x2000];

// Expands the escapes of p_text into g_expandedText and returns a copy of the result.
// Not 100%: the stack slots of the locals are permuted (i and length move past the buffer, which
// lengthens their encodings).
// FUNCTION: MW2SHELL 0x1002dc60
MechChar* ExpandTextEscapes(MechChar* p_text, MechChar* p_quote)
{
	MechS32 i;
	MechS32 length;
	MechChar number[0x80];
	MechS32 textLength;
	MechS32 rank;
	FILE* file;

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
	file = MechFopen("tmp.out", "wb");
	fwrite(p_text, 1, strlen(p_text), file);
	fclose(file);
	return AllocateString(g_expandedText);
}

// Lays out p_text (p_size bytes, the last of which becomes its terminator) on as many pages as
// it takes, adding them to p_pages. With p_quote, the text's escapes are expanded first.
// Not 100%: the stack slots of page, text and the new temporaries are permuted.
// FUNCTION: MW2SHELL 0x1002e09b
void LayoutTextPages(
	MechChar* p_text,
	MechS32 p_size,
	Collection* p_pages,
	Font* p_font,
	undefined* p_colors,
	MechS32 p_left,
	MechS32 p_top,
	MechS32 p_width,
	MechS32 p_height,
	MechChar* p_quote
)
{
	Page* page;
	MechChar* text;

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
// Not 100%: the stack slots of node and i are permuted.
// FUNCTION: MW2SHELL 0x1002e1b1
void LoadTextPages(
	Collection* p_pages,
	MechS32 p_left,
	MechS32 p_top,
	MechS32 p_width,
	MechS32 p_height,
	MechChar* p_name,
	Font* p_font,
	MechChar* p_quote
)
{
	MechS32* node = NULL;
	MechS32 i;

	g_textPageColors[0] = 0xff;
	g_textPageColors[1] = 1;
	for (i = 2; i < 0x100; i++) {
		g_textPageColors[i] = 0xff;
	}

	if (!g_projectArchive->LoadBwd(p_name)) {
		// The original does nothing about a missing file.
	}

	node = g_projectArchive->FindBwdNode(*(MechS32*) g_bwdTags[c_tagHtxt]);
	if (node) {
		LayoutTextPages(
			(MechChar*) (node + 2),
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
