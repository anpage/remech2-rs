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
	char* m_text;        // 0x00
	MechPoint m_textPos; // 0x04
	int32_t m_bottom;    // 0x0c
	Font* m_font;        // 0x10
	uint8_t* m_colors;   // 0x14
	HitRect* m_rect;     // 0x18
	TextGlyph* m_hover;  // 0x1c
	TextGlyph* m_label;  // 0x20
	int32_t m_id;        // 0x24
	uint8_t m_drawRect;  // 0x28
	uint8_t m_enabled;   // 0x29

	MenuEntry(
		int32_t p_left,
		int32_t p_top,
		int32_t p_right,
		int32_t p_bottom,
		int32_t p_id,
		uint8_t p_drawRect,
		VideoDriver* p_videoDriver,
		Font* p_font,
		char* p_text,
		MechPoint p_textPos,
		uint8_t* p_colors,
		uint8_t p_enabled
	);
	~MenuEntry();
	void DrawRect(VideoDriver* p_videoDriver);
	uint8_t HitTest(int32_t p_x, int32_t p_y);
};
#pragma pack()

// The font argument is ignored: the buttons always use g_buttonFont. m_colors maps colour 0
// to 0xff and 1 to 6, and leaves the others as they are.
ButtonMenu::ButtonMenu(
	VideoDriver* p_videoDriver,
	Font* p_font,
	uint8_t p_drawRect,
	MainMenuButton* p_buttons,
	int32_t p_count
)
{
	TextGlyph* label = NULL;
	int32_t i;
	char* text;
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
			true
		);
		item->m_label = label;
		ExpandCollection(m_items, item);
	}
}

ButtonMenu::~ButtonMenu()
{
	int32_t i;
	MenuEntry* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		delete item;
	}

	MechHeapFree(g_primaryHeap, m_items->m_items);
	MechHeapFree(g_primaryHeap, m_items);
}

void ButtonMenu::RemoveButtonsFrom(int32_t p_id)
{
	Collection* retained;
	int32_t i;
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

int32_t ButtonMenu::HitTest(int32_t p_x, int32_t p_y)
{
	int32_t id;
	int32_t i;
	MenuEntry* item;

	id = -1;
	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->HitTest(p_x, p_y) == true && item->m_enabled == true) {
			id = item->m_id;
		}
	}

	return id;
}

void ButtonMenu::DrawRects()
{
	int32_t i;
	MenuEntry* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->m_drawRect == true) {
			item->DrawRect(m_videoDriver);
		}
	}
}

void ButtonMenu::RemoveButton(int32_t p_id)
{
	int32_t i;
	MenuEntry* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->m_id == p_id) {
			CollectionRemove(m_items, item, false);
			delete item;
		}
	}
}

// Adds one button.
void ButtonMenu::AddButton(MainMenuButton p_button, int32_t p_id, uint8_t p_drawRect)
{
	TextGlyph* label = NULL;
	char* text = p_button.m_text;
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
		true
	);
	item->m_label = label;
	ExpandCollection(m_items, item);
}

void ButtonMenu::EnableButton(int32_t p_id)
{
	int32_t i;
	MenuEntry* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->m_id == p_id && !item->m_enabled) {
			item->m_enabled = true;
			if (item->m_label != NULL) {
				m_videoDriver->AddGlyph(item->m_label, 1);
				item->m_label->Draw();
			}
		}
	}
}

void ButtonMenu::DisableButton(int32_t p_id)
{
	int32_t i;
	MenuEntry* item;

	for (i = 0; i < m_items->m_count; i++) {
		item = (MenuEntry*) CollectionGet(m_items, i);
		if (item->m_id == p_id && item->m_enabled) {
			item->m_enabled = false;
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
	int32_t p_left,
	int32_t p_top,
	int32_t p_right,
	int32_t p_bottom,
	int32_t p_id,
	uint8_t p_drawRect,
	VideoDriver* p_videoDriver,
	Font* p_font,
	char* p_text,
	MechPoint p_textPos,
	uint8_t* p_colors,
	uint8_t p_enabled
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
	if (m_drawRect == true) {
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

uint8_t MenuEntry::HitTest(int32_t p_x, int32_t p_y)
{
	if (m_enabled && m_rect->Contains(p_x, p_y)) {
		if (m_hover == NULL && m_text != NULL && strlen(m_text) != 0) {
			m_hover = m_font->AddOverlayText(m_textPos.x, m_textPos.y, m_text, m_colors);
		}
		return true;
	}
	else {
		if (m_hover != NULL) {
			delete m_hover;
			m_hover = NULL;
			if (m_label != NULL) {
				m_label->Draw();
			}
		}

		return false;
	}
}

HitRect::HitRect(uint32_t p_left, uint32_t p_top, uint32_t p_right, uint32_t p_bottom)
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

uint8_t HitRect::Contains(int32_t p_x, int32_t p_y)
{
	if (m_bottom >= p_y && m_top <= p_y && m_right >= p_x && m_left <= p_x) {
		return true;
	}

	return false;
}
