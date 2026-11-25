#ifndef SCREENFIELD_H
#define SCREENFIELD_H

#include "types.h"

class TextGlyph;

// One labelled field of a shell screen. Screens keep them in tables ended by m_left == -1; the
// m_draw callback draws the field and returns its glyph. Its m_data depends on the callback:
// a int32_t* or char** to the value shown, a string, a flag, or an index.
struct ScreenField {
	int32_t m_left;
	int32_t m_top; // negative: packed row (bits 4-11) and offset (bits 0-3)
	int32_t m_width;
	int32_t m_height;
	uint8_t* m_colors; // color map for the glyph
	TextGlyph* m_glyph;
	TextGlyph* (*m_draw)(ScreenField*);
	void (*m_click)(ScreenField*); // click callback
	void* m_data;                  // the callback's data, see above
	ScreenField* m_arg;            // the table to switch to, or an integer (an item id, an axis kind)
};

#endif // SCREENFIELD_H
