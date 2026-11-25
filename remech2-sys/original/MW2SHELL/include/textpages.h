#ifndef TEXTPAGES_H
#define TEXTPAGES_H

#include "collection.h"
#include "decomp.h"
#include "font.h"
#include "types.h"

// The functions and globals of textpages.cpp that other units use.
extern uint8_t g_textPageColors[0x100];
extern char g_expandedText[0x2000];
void LoadTextPages(
	Collection* p_pages,
	int32_t p_left,
	int32_t p_top,
	int32_t p_width,
	int32_t p_height,
	char* p_name,
	Font* p_font,
	char* p_quote
);

#endif // TEXTPAGES_H
