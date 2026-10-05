#ifndef TEXTPAGES_H
#define TEXTPAGES_H

#include "collection.h"
#include "decomp.h"
#include "font.h"
#include "types.h"

// The functions and globals of textpages.cpp that other units use.
extern MechU8 g_textPageColors[0x100];
extern MechChar g_expandedText[0x2000];
void LoadTextPages(
	Collection* p_pages,
	MechS32 p_left,
	MechS32 p_top,
	MechS32 p_width,
	MechS32 p_height,
	MechChar* p_name,
	Font* p_font,
	MechChar* p_quote
);

#endif // TEXTPAGES_H
