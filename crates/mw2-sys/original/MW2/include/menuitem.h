#ifndef MENUITEM_H
#define MENUITEM_H

#include "decomp.h"
#include "point.h"
#include "types.h"

struct MenuControl;
struct MenuDefinition;
struct MenuPage;

// Draws a menu item's control and acts on the menu key for it (RunMenuItems calls it for every
// item, at the page's control column).
typedef void (*MenuItemRunFn)(
	struct MenuDefinition* p_menu,
	struct MenuControl* p_control,
	MechS32 p_index,
	Point p_pos,
	struct MenuPage* p_page
);

// An item of a menu page.
// SIZE 0x14
typedef struct MenuItem {
	MechS32 m_type; // 0x00 — 0: opens m_subpage, 2: accepts (and goes back), 3: a heading without a
					// number, 5 and 6: close the page (ApplyMenuKey)
	MechChar* m_text;              // 0x04
	MenuItemRunFn m_run;           // 0x08
	struct MenuControl* m_control; // 0x0c — passed to m_run
	struct MenuPage* m_subpage;    // 0x10
} MenuItem;

#endif // MENUITEM_H
