#include "textglyphlist.h"

#include "textglyph.h"
#include "windowstate.h"

DECOMP_SIZE_ASSERT(TextGlyphList, 0x04)

// FUNCTION: MW2SHELL 0x1003e100
TextGlyphList::TextGlyphList()
{
	CreateCollection(&m_items, 25, NULL, 4, NULL);
}

// FUNCTION: MW2SHELL 0x1003e12d
TextGlyphList::~TextGlyphList()
{
	MechHeapFree(g_primaryHeap, m_items->m_items);
	MechHeapFree(g_primaryHeap, m_items->m_items);
}

// FUNCTION: MW2SHELL 0x1003e171
void TextGlyphList::Add(TextGlyph* p_item)
{
	ExpandCollection(m_items, p_item);
}

// FUNCTION: MW2SHELL 0x1003e19b
void TextGlyphList::Remove(TextGlyph* p_item)
{
	MechS32 index;

	index = CollectionFind(m_items, p_item);
	if (index >= 0) {
		CollectionRemove(m_items, p_item, FALSE);
	}
}

// FUNCTION: MW2SHELL 0x1003e1e6
void TextGlyphList::Clear(MechU8 p_delete)
{
	TextGlyph* item;

	while (m_items->m_count != 0) {
		item = (TextGlyph*) CollectionGet(m_items, m_items->m_count - 1);
		if (item != NULL) {
			if (p_delete == TRUE) {
				delete item;
			}
			else {
				item->Shutdown();
			}
		}
	}
}

// FUNCTION: MW2SHELL 0x1003e286
void TextGlyphList::DrawAll()
{
	MechS32 i;
	TextGlyph* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (TextGlyph*) CollectionGet(m_items, i);
		if (item != NULL) {
			item->Draw();
		}
	}
}
