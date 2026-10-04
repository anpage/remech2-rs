#ifndef TEXTGLYPHLIST_H
#define TEXTGLYPHLIST_H

#include "collection.h"
#include "decomp.h"
#include "types.h"

class TextGlyph;

// SIZE 0x04
class TextGlyphList {
public:
	TextGlyphList();
	~TextGlyphList();

	void Add(TextGlyph* p_item);
	void Remove(TextGlyph* p_item);
	void Clear(MechU8 p_delete);
	void DrawAll();

private:
	Collection* m_items; // 0x00
};

#endif // TEXTGLYPHLIST_H
