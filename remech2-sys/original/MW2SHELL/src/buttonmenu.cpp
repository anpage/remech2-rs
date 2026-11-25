#include "buttonmenu.h"

#include "collection.h"
#include "font.h"
#include "hitrect.h"
#include "mainmenubutton.h"
#include "shellglobals.h"
#include "textglyph.h"
#include "videodriver.h"
#include "windowstate.h"

#include <string.h>

// The menu's collection owns 0x2a-byte entries with a hit rectangle and two text glyphs.
#pragma pack(1)
struct MenuEntry {
	MechChar* m_text;    // 0x00
	MechPoint m_textPos; // 0x04
	MechS32 m_bottom;    // 0x0c
	Font* m_font;        // 0x10
	undefined* m_colors; // 0x14
	HitRect* m_rect;     // 0x18
	TextGlyph* m_hover;  // 0x1c
	TextGlyph* m_label;  // 0x20
	MechS32 m_id;        // 0x24
	MechU8 m_drawRect;   // 0x28
	MechU8 m_enabled;    // 0x29

	MenuEntry(
		MechS32 p_left,
		MechS32 p_top,
		MechS32 p_right,
		MechS32 p_bottom,
		MechS32 p_id,
		MechU8 p_drawRect,
		VideoDriver* p_videoDriver,
		Font* p_font,
		MechChar* p_text,
		MechPoint p_textPos,
		undefined* p_colors,
		MechU8 p_enabled
	);
	~MenuEntry();
	void DrawRect(VideoDriver* p_videoDriver);
	MechU8 HitTest(MechS32 p_x, MechS32 p_y);
};
#pragma pack()

// The font argument is ignored: the buttons always use g_buttonFont. m_colors maps colour 0
// to 0xff and 1 to 6, and leaves the others as they are.
ButtonMenu::ButtonMenu(
	VideoDriver* p_videoDriver,
	Font* p_font,
	MechU8 p_drawRect,
	MainMenuButton* p_buttons,
	MechS32 p_count
)
{
	TextGlyph* label = NULL;
	MechS32 i;
	MechChar* text;
	MenuEntry* item;

	p_font = g_buttonFont;
	m_drawRect = p_drawRect;
	m_videoDriver = p_videoDriver;
	m_font = p_font;

	m_colors[0] = 0xff;
	for (i = 1; i < 0x100; i++) {
		m_colors[i] = i;
	}
	m_colors[1] = 6;

	CreateCollection(&m_items, 10, NULL, 4, NULL);
	for (i = 0; i < p_count; i++) {
		text = p_buttons[i].m_text;
		if (text != NULL && *text == '<') {
			text++;
			label = m_font->AddOverlayText(p_buttons[i].m_textPos.x, p_buttons[i].m_textPos.y, text, m_colors);
		}
		else {
			label = NULL;
		}

		item = new MenuEntry(
			p_buttons[i].m_left,
			p_buttons[i].m_top,
			p_buttons[i].m_right,
			p_buttons[i].m_bottom,
			i,
			m_drawRect,
			m_videoDriver,
			m_font,
			text,
			p_buttons[i].m_textPos,
			NULL,
			TRUE
		);
		item->m_label = label;
		ExpandCollection(m_items, item);
	}
}

ButtonMenu::~ButtonMenu()
{
	MechS32 i;
	MenuEntry* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		delete item;
	}

	MechHeapFree(g_primaryHeap, m_items->m_items);
	MechHeapFree(g_primaryHeap, m_items);
}

void ButtonMenu::RemoveButtonsFrom(MechS32 p_id)
{
	Collection* retained;
	MechS32 i;
	MenuEntry* item;

	CreateCollection(&retained, 10, NULL, 4, NULL);
	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->m_id >= p_id) {
			delete item;
		}
		else {
			ExpandCollection(retained, item);
		}
		m_items->m_items[i] = NULL;
	}

	ClearCollection(m_items);
	MoveCollectionItems(m_items, retained);
	DestroyCollection(retained);
}

MechS32 ButtonMenu::HitTest(MechS32 p_x, MechS32 p_y)
{
	MechS32 id;
	MechS32 i;
	MenuEntry* item;

	id = -1;
	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->HitTest(p_x, p_y) == TRUE && item->m_enabled == TRUE) {
			id = item->m_id;
		}
	}

	return id;
}

void ButtonMenu::DrawRects()
{
	MechS32 i;
	MenuEntry* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->m_drawRect == TRUE) {
			item->DrawRect(m_videoDriver);
		}
	}
}

void ButtonMenu::RemoveButton(MechS32 p_id)
{
	MechS32 i;
	MenuEntry* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->m_id == p_id) {
			CollectionRemove(m_items, item, FALSE);
			delete item;
		}
	}
}

// Adds one button.
void ButtonMenu::AddButton(MainMenuButton p_button, MechS32 p_id, MechU8 p_drawRect)
{
	TextGlyph* label = NULL;
	MechChar* text = p_button.m_text;
	MenuEntry* item;

	if (text != NULL && *text == '<') {
		text++;
		label = m_font->AddOverlayText(p_button.m_textPos.x, p_button.m_textPos.y, text, m_colors);
	}

	item = new MenuEntry(
		p_button.m_left,
		p_button.m_top,
		p_button.m_right,
		p_button.m_bottom,
		p_id,
		p_drawRect,
		m_videoDriver,
		m_font,
		text,
		p_button.m_textPos,
		NULL,
		TRUE
	);
	item->m_label = label;
	ExpandCollection(m_items, item);
}

void ButtonMenu::EnableButton(MechS32 p_id)
{
	MechS32 i;
	MenuEntry* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->m_id == p_id && !item->m_enabled) {
			item->m_enabled = TRUE;
			if (item->m_label != NULL) {
				m_videoDriver->AddGlyph(item->m_label, 1);
				item->m_label->Draw();
			}
		}
	}
}

void ButtonMenu::DisableButton(MechS32 p_id)
{
	MechS32 i;
	MenuEntry* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->m_id == p_id && item->m_enabled) {
			item->m_enabled = FALSE;
			if (item->m_hover != NULL) {
				delete item->m_hover;
				item->m_hover = NULL;
			}
			if (item->m_label != NULL) {
				item->m_label->Shutdown();
			}
		}
	}
}

MenuEntry::MenuEntry(
	MechS32 p_left,
	MechS32 p_top,
	MechS32 p_right,
	MechS32 p_bottom,
	MechS32 p_id,
	MechU8 p_drawRect,
	VideoDriver* p_videoDriver,
	Font* p_font,
	MechChar* p_text,
	MechPoint p_textPos,
	undefined* p_colors,
	MechU8 p_enabled
)
{
	m_id = p_id;
	m_drawRect = p_drawRect;
	m_text = p_text;
	m_textPos = p_textPos;
	m_hover = NULL;
	m_font = p_font;
	m_bottom = -1;
	m_colors = p_colors;
	m_enabled = p_enabled;
	m_rect = new HitRect(p_left, p_top, p_right, p_bottom);
	if (m_drawRect == TRUE) {
		DrawRect(p_videoDriver);
	}
}

MenuEntry::~MenuEntry()
{
	if (m_hover != NULL) {
		delete m_hover;
	}
	if (m_label != NULL) {
		delete m_label;
	}
}

void MenuEntry::DrawRect(VideoDriver* p_videoDriver)
{
	m_rect->Draw(p_videoDriver);
}

MechU8 MenuEntry::HitTest(MechS32 p_x, MechS32 p_y)
{
	if (m_enabled && m_rect->Contains(p_x, p_y)) {
		if (m_hover == NULL && m_text != NULL && strlen(m_text) != 0) {
			m_hover = m_font->AddOverlayText(m_textPos.x, m_textPos.y, m_text, m_colors);
		}
		return TRUE;
	}
	else {
		if (m_hover != NULL) {
			delete m_hover;
			m_hover = NULL;
			if (m_label != NULL) {
				m_label->Draw();
			}
		}

		return FALSE;
	}
}

HitRect::HitRect(undefined4 p_left, undefined4 p_top, undefined4 p_right, undefined4 p_bottom)
{
	m_left = p_left;
	m_top = p_top;
	m_right = p_right;
	m_bottom = p_bottom;
}

void HitRect::Draw(VideoDriver* p_videoDriver)
{
	p_videoDriver->DrawLine(m_left, m_bottom, m_right, m_bottom, 1);
	p_videoDriver->DrawLine(m_left, m_top, m_right, m_top, 1);
	p_videoDriver->DrawLine(m_right, m_top, m_right, m_bottom, 1);
	p_videoDriver->DrawLine(m_left, m_bottom, m_left, m_top, 1);
}

MechU8 HitRect::Contains(MechS32 p_x, MechS32 p_y)
{
	if (m_bottom >= p_y && m_top <= p_y && m_right >= p_x && m_left <= p_x) {
		return TRUE;
	}

	return FALSE;
}
