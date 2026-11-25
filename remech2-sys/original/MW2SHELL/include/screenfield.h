#ifndef SCREENFIELD_H
#define SCREENFIELD_H

#include "decomp.h"
#include "types.h"

class TextGlyph;

// One labelled field of a shell screen. Screens keep them in tables ended by m_left == -1; the
// callback at 0x1c draws the field and returns its glyph. Its data at 0x24 depends on the callback:
// a int32_t* or char** to the value shown, a string, a flag, or an index.
struct ScreenField {
	int32_t m_left;                     // 0x00
	int32_t m_top;                      // 0x04 — negative: packed row (bits 4-11) and offset (bits 0-3)
	int32_t m_width;                    // 0x08
	int32_t m_height;                   // 0x0c
	undefined* m_colors;                // 0x14 — color map for the glyph
	TextGlyph* m_glyph;                 // 0x18
	TextGlyph* (*m_draw)(ScreenField*); // 0x1c
	void (*m_click)(ScreenField*);      // 0x20 — click callback
	void* m_data;                       // 0x24 — the callback's data, see above
	ScreenField* m_arg;                 // 0x28 — the table to switch to, or an integer (an item id, an axis kind)
};

#endif // SCREENFIELD_H
