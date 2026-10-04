#ifndef MENUPAGE_H
#define MENUPAGE_H

#include "decomp.h"
#include "menuitem.h"
#include "types.h"

struct MenuDefinition;
struct MenuPage;

// Prepares a page when its menu is first laid out (FirstMenu); a menu whose pages don't all
// return TRUE is dropped.
typedef MechS32 (*MenuPageInitFn)(struct MenuDefinition* p_menu, struct MenuPage* p_page);

// A page of a menu: its items, some of which the page's callbacks copy from templates at the
// end of the table.
// SIZE 0x158
typedef struct MenuPage {
	MechS32 m_state;       // 0x00 — 0: new, 1: opening, 2: open, 3: opening a subpage, 4: accepted,
						   // 5: cancelled (RunMenuItems)
	MechChar* m_title;     // 0x04
	MechU32 m_aiSlot;      // 0x08 — an AI slot (FindStarSlotPlayer)
	MechS32 m_itemCount;   // 0x0c
	MechS32 m_selected;    // 0x10 — the highlighted item
	MenuPageInitFn m_init; // 0x14
	MenuItem m_items[16];  // 0x18
} MenuPage;

#endif // MENUPAGE_H
