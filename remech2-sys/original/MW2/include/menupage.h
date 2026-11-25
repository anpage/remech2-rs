#ifndef MENUPAGE_H
#define MENUPAGE_H

#include "menuitem.h"
#include "types.h"

struct MenuDefinition;
struct MenuPage;

// Prepares a page when its menu is first laid out (FirstMenu); a menu whose pages don't all
// return true is dropped.
typedef int32_t (*MenuPageInitFn)(struct MenuDefinition* p_menu, struct MenuPage* p_page);

// A page of a menu: its items, some of which the page's callbacks copy from templates at the
// end of the table.
typedef struct MenuPage {
	int32_t m_state;       // 0x00 — 0: new, 1: opening, 2: open, 3: opening a subpage, 4: accepted,
						   // 5: cancelled (RunMenuItems)
	char* m_title;         // 0x04
	uint32_t m_aiSlot;     // 0x08 — an AI slot (FindStarSlotPlayer)
	int32_t m_itemCount;   // 0x0c
	int32_t m_selected;    // 0x10 — the highlighted item
	MenuPageInitFn m_init; // 0x14
	MenuItem m_items[16];  // 0x18
} MenuPage;

#endif // MENUPAGE_H
