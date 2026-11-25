#ifndef MENU_H
#define MENU_H

#include "point.h"
#include "targeting.h"
#include "types.h"

struct MenuPage;

// An in-mission menu's definition, one of the eleven in g_menuDefinitions.
typedef struct MenuDefinition {
	PANE* m_target;                // where the menu draws
	uint32_t m_flags;              // 1: takes navigation keys, 0x20: clears its target
	struct MenuPage** m_pageStack; // the open pages, up to 8
	int32_t m_pageDepth;
	int32_t m_backgroundId; // a SHP resource, -1: none
	void* m_background;
	PANE* m_backgroundTarget;
	int32_t m_cursorShapeId; // a SHP resource, -1: none
	void* m_cursorShape;
	int32_t m_openSound; // played on opening a subpage, -1: none
	int32_t m_moveSound; // played on moving the selection, -1: none
	int32_t m_fontId;    // a FONT resource
	void* m_font;
	int32_t m_color;
	int32_t m_highlightColor; // the selected item's
	int32_t m_lineCount;      // lines, for the line spacing
	Point m_textOrigin;       // the text origin, in pixels; m_y: the line spacing
	Point m_titleOrigin;      // the title; m_y: half the line spacing
	Point m_cursorOrigin;     // the selection cursor; m_y: the line spacing
	Point m_itemOrigin;       // the items' numbers and texts; m_y: the line spacing
	Point m_controlOrigin;    // the items' controls; m_y: the line spacing
	struct MenuPage* m_rootPage;
} MenuDefinition;

// A registered menu: RegisterMenu adds one per menu ID to g_menuSlots.
typedef struct MenuSlot {
	int32_t m_id;                 // the index in g_menuDefinitions
	int32_t m_state;              // 1: open
	int32_t m_requested;          // the state UpdateMenus moves it to
	MenuDefinition* m_definition; // while open
	struct MenuSlot* m_next;
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
