#include "textglyphlist.h"

#include "textglyph.h"
#include "windowstate.h"

TextGlyphList::TextGlyphList()
{
	CreateCollection(&m_items, 25, NULL, 4, NULL);
}

TextGlyphList::~TextGlyphList()
{
	MechHeapFree(g_primaryHeap, m_items->m_items);
	MechHeapFree(g_primaryHeap, m_items->m_items);
}

void TextGlyphList::Add(TextGlyph* p_item)
{
	ExpandCollection(m_items, p_item);
}

void TextGlyphList::Remove(TextGlyph* p_item)
{
	int32_t index;

	index = CollectionFind(m_items, p_item);
	if (index >= 0) {
		CollectionRemove(m_items, p_item, false);
	}
}

void TextGlyphList::Clear(uint8_t p_delete)
{
	TextGlyph* item;

	while (m_items->m_count != 0) {
		item = (TextGlyph*) CollectionGet(m_items, m_items->m_count - 1);
		if (item != NULL) {
			if (p_delete == true) {
				delete item;
			}
			else {
				item->Shutdown();
			}
		}
	}
}

void TextGlyphList::DrawAll()
{
	int32_t i;
	TextGlyph* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (TextGlyph*) CollectionGet(m_items, i);
		if (item != NULL) {
			item->Draw();
		}
	}
}
