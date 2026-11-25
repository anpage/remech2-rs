#ifndef MENU_H
#define MENU_H

#include "point.h"
#include "targeting.h"
#include "types.h"

struct MenuPage;

// An in-mission menu's definition, one of the eleven in g_menuDefinitions.
typedef struct MenuDefinition {
	PANE* m_target;                // 0x00 — where the menu draws
	uint32_t m_flags;              // 0x04 — 1: takes navigation keys, 0x20: clears its target
	struct MenuPage** m_pageStack; // 0x08 — the open pages, up to 8
	int32_t m_pageDepth;           // 0x0c
	int32_t m_backgroundId;        // 0x10 — a SHP resource, -1: none
	void* m_background;            // 0x14
	PANE* m_backgroundTarget;      // 0x18
	int32_t m_cursorShapeId;       // 0x1c — a SHP resource, -1: none
	void* m_cursorShape;           // 0x20
	int32_t m_openSound;           // 0x24 — played on opening a subpage, -1: none
	int32_t m_moveSound;           // 0x28 — played on moving the selection, -1: none
	int32_t m_fontId;              // 0x2c — a FONT resource
	void* m_font;                  // 0x30
	int32_t m_color;               // 0x34
	int32_t m_highlightColor;      // 0x38 — the selected item's
	int32_t m_lineCount;           // 0x3c — lines, for the line spacing
	Point m_textOrigin;            // 0x40 — the text origin, in pixels; m_y: the line spacing
	Point m_titleOrigin;           // 0x48 — the title; m_y: half the line spacing
	Point m_cursorOrigin;          // 0x50 — the selection cursor; m_y: the line spacing
	Point m_itemOrigin;            // 0x58 — the items' numbers and texts; m_y: the line spacing
	Point m_controlOrigin;         // 0x60 — the items' controls; m_y: the line spacing
	struct MenuPage* m_rootPage;   // 0x68
} MenuDefinition;

// A registered menu: RegisterMenu adds one per menu ID to g_menuSlots.
typedef struct MenuSlot {
	int32_t m_id;                 // 0x00 — the index in g_menuDefinitions
	int32_t m_state;              // 0x04 — 1: open
	int32_t m_requested;          // 0x08 — the state UpdateMenus moves it to
	MenuDefinition* m_definition; // 0x0c — while open
	struct MenuSlot* m_next;      // 0x14
} MenuSlot;

// The functions and globals of menu.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern uint8_t g_textColors[0x100];
	extern int32_t g_menuRepeatTimer;
	extern int32_t g_menuKey;
	extern MenuDefinition* g_menuDefinitions[11];
	extern int32_t g_openMenuCount;
	extern MenuSlot* g_menuSlotsTail;
	extern MenuSlot* g_menuSlots;

	int32_t RegisterMenu(int32_t p_id);
	int32_t RequestMenu(int32_t p_id);
	int32_t ToggleMenu(int32_t p_id);
	void FreeMenus(void);
	void FirstMenu(void);
	void UpdateMenuKey(void);
	MenuDefinition* GetOpenMenu(void);
	void UpdateMenus(void);
	int32_t GetMenuSlotState(int32_t p_id);
	void RequestMenuClose(int32_t p_id);

#ifdef __cplusplus
}
#endif

#endif // MENU_H
