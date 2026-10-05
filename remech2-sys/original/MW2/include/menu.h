#ifndef MENU_H
#define MENU_H

#include "decomp.h"
#include "point.h"
#include "targeting.h"
#include "types.h"

struct MenuPage;

// An in-mission menu's definition, one of the eleven in g_menuDefinitions.
// SIZE 0x6c
typedef struct MenuDefinition {
	PANE* m_target;                // 0x00 — where the menu draws
	MechU32 m_flags;               // 0x04 — 1: takes navigation keys, 0x20: clears its target
	struct MenuPage** m_pageStack; // 0x08 — the open pages, up to 8
	MechS32 m_pageDepth;           // 0x0c
	MechS32 m_backgroundId;        // 0x10 — a SHP resource, -1: none
	void* m_background;            // 0x14
	PANE* m_backgroundTarget;      // 0x18
	MechS32 m_cursorShapeId;       // 0x1c — a SHP resource, -1: none
	void* m_cursorShape;           // 0x20
	MechS32 m_openSound;           // 0x24 — played on opening a subpage, -1: none
	MechS32 m_moveSound;           // 0x28 — played on moving the selection, -1: none
	MechS32 m_fontId;              // 0x2c — a FONT resource
	void* m_font;                  // 0x30
	MechS32 m_color;               // 0x34
	MechS32 m_highlightColor;      // 0x38 — the selected item's
	MechS32 m_lineCount;           // 0x3c — lines, for the line spacing
	Point m_textOrigin;            // 0x40 — the text origin, in pixels; m_y: the line spacing
	Point m_titleOrigin;           // 0x48 — the title; m_y: half the line spacing
	Point m_cursorOrigin;          // 0x50 — the selection cursor; m_y: the line spacing
	Point m_itemOrigin;            // 0x58 — the items' numbers and texts; m_y: the line spacing
	Point m_controlOrigin;         // 0x60 — the items' controls; m_y: the line spacing
	struct MenuPage* m_rootPage;   // 0x68
} MenuDefinition;

// A registered menu: RegisterMenu adds one per menu ID to g_menuSlots.
// SIZE 0x18
typedef struct MenuSlot {
	MechS32 m_id;                 // 0x00 — the index in g_menuDefinitions
	MechS32 m_state;              // 0x04 — 1: open
	MechS32 m_requested;          // 0x08 — the state UpdateMenus moves it to
	MenuDefinition* m_definition; // 0x0c — while open
	undefined4 m_unk0x10;         // 0x10
	struct MenuSlot* m_next;      // 0x14
} MenuSlot;

// The functions and globals of menu.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern undefined g_textColors[0x100];
	extern MechS32 g_menuRepeatTimer;
	extern MechS32 g_menuKey;
	extern MenuDefinition* g_menuDefinitions[11];
	extern MechS32 g_openMenuCount;
	extern MenuSlot* g_menuSlotsTail;
	extern MenuSlot* g_menuSlots;

	MechS32 RegisterMenu(MechS32 p_id);
	MechS32 RequestMenu(MechS32 p_id);
	MechS32 ToggleMenu(MechS32 p_id);
	void FreeMenus(void);
	void FirstMenu(void);
	void UpdateMenuKey(void);
	MenuDefinition* GetOpenMenu(void);
	void UpdateMenus(void);
	MechS32 GetMenuSlotState(MechS32 p_id);
	void RequestMenuClose(MechS32 p_id);

#ifdef __cplusplus
}
#endif

#endif // MENU_H
