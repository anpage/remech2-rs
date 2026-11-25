#include "menu.h"

#include "commandmenu.h"
#include "commandpointmenu.h"
#include "decomp.h"
#include "dorcs.h"
#include "fixeddiv.h"
#include "inputmap.h"
#include "loadres.h"
#include "mainmenu.h"
#include "menucontrol.h"
#include "menupage.h"
#include "mw2prj.h"
#include "render.h"
#include "screenscale.h"
#include "setres.h"
#include "settings.h"
#include "simmain.h"
#include "soundfx.h"
#include "targeting.h"
#include "ticks.h"
#include "types.h"
#include "vfxa.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int32_t g_menuRepeatTimer = -1;

// The in-mission menus by ID (RegisterMenu): 4 the main menu, 5 the systems status, 1 the lance
// command computer, 7 and 8 command points 2 and 3, 3 the programmers' page.
MenuDefinition* g_menuDefinitions[11] = {
	NULL,
	&g_commandMenu,
	NULL,
	&g_dorcsMenu,
	&g_mainMenu,
	&g_systemsMenu,
	NULL,
	&g_commandPoint2Menu,
	&g_commandPoint3Menu,
	NULL,
	NULL,
};

undefined g_textColors[0x100];

int32_t g_openMenuCount;

MenuSlot* g_menuSlotsTail;

MenuSlot* g_menuSlots;

// The key the open menu acts on this frame (0: none).
int32_t g_menuKey;

int32_t ActivateMenu(MenuSlot* p_slot);
void DeactivateMenu(MenuSlot* p_slot);
int32_t DrawAndRunMenu(MenuDefinition* p_menu);
void RunMenuItems(MenuDefinition* p_menu);
MenuSlot* FindMenuSlot(int32_t p_id);
MenuPage* PeekMenuPage(MenuDefinition* p_menu);
int32_t PushMenuPage(MenuDefinition* p_menu, MenuPage* p_page);
MenuPage* PopMenuPage(MenuDefinition* p_menu);
void ClearMenuPages(MenuDefinition* p_menu);
int32_t IsMenuPageStackEmpty(MenuDefinition* p_menu);

// Adds a slot for a menu ID at the end of g_menuSlots. Returns whether it could.
int32_t RegisterMenu(int32_t p_id)
{
	int32_t result;
	MenuSlot* slot;

	result = 0;
	slot = MechHeapAlloc(g_primaryHeap, sizeof(MenuSlot));
	if (slot == NULL) {
		return result;
	}

	memset(slot, 0, sizeof(MenuSlot));
	slot->m_id = p_id;
	if (g_menuSlots == NULL) {
		g_menuSlots = slot;
	}
	else {
		g_menuSlotsTail->m_next = slot;
	}
	g_menuSlotsTail = slot;
	result = 1;

	return result;
}

// Asks for an in-mission menu to open (4 is the mission menu), if no menu is open already.
// Returns whether it will.
int32_t RequestMenu(int32_t p_id)
{
	MenuSlot* slot;
	int32_t result;

	result = 0;
	if (g_openMenuCount == 0) {
		slot = FindMenuSlot(p_id);
		if (slot) {
			slot->m_requested = 1;
			result = 1;
		}
	}

	return result;
}

// Asks for a closed menu to open or an open one to close. Returns whether it asked to open it.
int32_t ToggleMenu(int32_t p_id)
{
	int32_t result;

	result = 0;
	switch (GetMenuSlotState(p_id)) {
	case 1:
		RequestMenuClose(p_id);
		break;
	case 0:
		RequestMenu(p_id);
		result = 1;
		break;
	default:
		break;
	}

	return result;
}

void FreeMenus(void)
{
	MenuSlot* slot;
	MenuSlot* freed;

	slot = g_menuSlots;
	while (slot) {
		DeactivateMenu(slot);
		freed = slot;
		slot = slot->m_next;
		MechHeapFree(g_primaryHeap, freed);
	}

	g_menuSlots = NULL;
	g_menuSlotsTail = NULL;
}

// Lays out a menu on the screen: scales its background target to the background shape, spaces
// its m_lineCount lines evenly down the target, and converts its points to pixels. With flag 0x10
// the background moves to the target's left edge.
void LayoutMenu(MenuDefinition* p_menu)
{
	Point scale;
	void* shape;
	int32_t dx;
	int32_t size;
	PANE* target;
	Point origin;
	PANE* background;

	target = p_menu->m_target;
	if (!target) {
		return;
	}

	background = p_menu->m_backgroundTarget;
	if (!background) {
		return;
	}

	if (!target->m_window) {
		return;
	}

	if (!background->m_window) {
		return;
	}

	if (p_menu->m_backgroundId != -1) {
		shape = LoadCachedResource(
			g_mw2PrjHandle,
			p_menu->m_backgroundId + g_artResolution,
			g_resourceTypeTags[c_resTagShp],
			0
		);
		if (shape) {
			size = VFX_shape_bounds(shape, 0);
			UnlockCachedResource(p_menu->m_backgroundId + g_artResolution, g_resourceTypeTags[c_resTagShp]);
			scale.m_x = size >> 16;
			scale.m_y = size & 0xffff;
			scale.m_x = FixedDiv16(scale.m_x, g_screenWidthMinus1 + 1);
			scale.m_y = FixedDiv16(scale.m_y, g_screenHeightMinus1 + 1);
			scale.m_x = FixedDiv16(scale.m_x, background->m_x1 - background->m_x0 + 1);
			scale.m_y = FixedDiv16(scale.m_y, background->m_y1 - background->m_y0 + 1);
			ScaleRectAboutCenter(background, background, scale);
		}
	}

	origin.m_x = 0;
	origin.m_y = FixedDiv16(1, p_menu->m_lineCount + 2);
	p_menu->m_titleOrigin.m_y = FixedDiv16(origin.m_y, 0x20000);
	p_menu->m_cursorOrigin.m_y = p_menu->m_itemOrigin.m_y = p_menu->m_controlOrigin.m_y = origin.m_y + origin.m_y;
	ScaleRectToScreen(target->m_window, target, target);
	ScaleRectToScreen(background->m_window, background, background);
	ScalePointToFrame(target, &origin, &origin);
	ScalePointToFrame(target, &p_menu->m_titleOrigin, &p_menu->m_titleOrigin);
	ScalePointToFrame(target, &p_menu->m_cursorOrigin, &p_menu->m_cursorOrigin);
	ScalePointToFrame(target, &p_menu->m_itemOrigin, &p_menu->m_itemOrigin);
	ScalePointToFrame(target, &p_menu->m_controlOrigin, &p_menu->m_controlOrigin);
	if (p_menu->m_flags & 0x10) {
		dx = background->m_x0 - target->m_x0;
		background->m_x0 -= dx;
		background->m_x1 -= dx;
	}

	p_menu->m_textOrigin = origin;
}

// Loads a menu's background shapes and font.
void LoadMenuResources(MenuDefinition* p_menu)
{
	if (p_menu->m_backgroundId != -1) {
		p_menu->m_background = LoadCachedResource(
			g_mw2PrjHandle,
			p_menu->m_backgroundId + g_artResolution,
			g_resourceTypeTags[c_resTagShp],
			0
		);
	}
	else {
		p_menu->m_background = NULL;
	}

	if (p_menu->m_cursorShapeId != -1) {
		p_menu->m_cursorShape = LoadCachedResource(
			g_mw2PrjHandle,
			p_menu->m_cursorShapeId + g_artResolution,
			g_resourceTypeTags[c_resTagShp],
			0
		);
	}
	else {
		p_menu->m_cursorShape = NULL;
	}

	p_menu->m_font =
		LoadCachedResource(g_mw2PrjHandle, p_menu->m_fontId + g_artResolution, g_resourceTypeTags[c_resTagFont], 0);
}

// Opens a menu on its root page. Menus with flag 1 take the controls, so this calls
// DisableGameplayInput; DeactivateMenu calls EnableGameplayInput again. Returns whether the menu
// has a definition (the original reads the definition uninitialized for a menu ID outside 1-10).
int32_t ActivateMenu(MenuSlot* p_slot)
{
	int32_t result;
	MenuDefinition* menu;

	result = 0;
	if (g_menuRepeatTimer == -1) {
		g_menuRepeatTimer = AllocTicks(0x100);
	}

	ResetTicks(g_menuRepeatTimer);
	if (p_slot->m_id >= 1 && p_slot->m_id <= 10) {
		p_slot->m_definition = g_menuDefinitions[p_slot->m_id];
		menu = p_slot->m_definition;
	}

	if (!menu) {
		return result;
	}

	ClearMenuPages(menu);
	PushMenuPage(menu, menu->m_rootPage);
	g_openMenuCount++;
	if (menu->m_flags & 1) {
		DisableGameplayInput();
	}

	result = 1;
	return result;
}

// Closes a menu and frees its resources.
void DeactivateMenu(MenuSlot* p_slot)
{
	MenuDefinition* menu;

	menu = p_slot->m_definition;
	if (menu) {
		if (menu->m_background) {
			UnlockCachedResource(menu->m_backgroundId + g_artResolution, g_resourceTypeTags[c_resTagShp]);
			menu->m_background = NULL;
		}

		if (menu->m_cursorShape) {
			UnlockCachedResource(menu->m_cursorShapeId + g_artResolution, g_resourceTypeTags[c_resTagShp]);
			menu->m_cursorShape = NULL;
		}

		if (menu->m_font) {
			UnlockCachedResource(menu->m_fontId + g_artResolution, g_resourceTypeTags[c_resTagFont]);
			menu->m_font = NULL;
		}

		if (menu->m_flags & 1) {
			EnableGameplayInput();
		}
	}

	p_slot->m_definition = NULL;
	g_openMenuCount--;
}

// Asks for a menu to close.
void RequestMenuClose(int32_t p_id)
{
	MenuSlot* slot;

	slot = FindMenuSlot(p_id);
	if (slot) {
		slot->m_requested = 0;
	}
}

// First draws the menus' panes to the main pixel buffer and loads their layout; drops
// the definitions whose root page or its subpages fail their init callbacks.
void FirstMenu(void)
{
	int32_t ok;
	MenuDefinition* menu;
	MenuPage* subpage;
	int32_t i;
	int32_t j;
	int32_t count;
	MenuPage* page;

	for (i = 0; i <= 10; i++) {
		menu = g_menuDefinitions[i];
		if (menu) {
			ok = 0;
			if (menu->m_target && menu->m_backgroundTarget) {
				menu->m_backgroundTarget->m_window = &g_mainPixelBuffer;
				menu->m_target->m_window = menu->m_backgroundTarget->m_window;
				LayoutMenu(menu);
				page = menu->m_rootPage;
				if (page) {
					ok = 1;
					if (page->m_init) {
						ok &= page->m_init(menu, page);
					}

					count = page->m_itemCount;
					for (j = 0; j < count; j++) {
						subpage = page->m_items[j].m_subpage;
						if (subpage && subpage->m_init) {
							ok &= subpage->m_init(menu, subpage);
						}
					}
				}
			}

			if (ok != 1) {
				g_menuDefinitions[i] = NULL;
			}
		}
	}
}

// Works out g_menuKey for the open menu: the digits from the key code, Escape and (in menus that
// take navigation keys) the rest from the bindings, else from the menu sinks (repeating while they
// are held). The typed keys the original took from the key code are dropped.
void UpdateMenuKey(void)
{
	MenuDefinition* menu;
	uint32_t flags;
	int16_t bound;

	g_menuKey = 0;
	bound = TakeBoundMenuKey();
	menu = GetOpenMenu();
	if (menu) {
		flags = menu->m_flags;
		if (g_localSteering.m_keyCode >= '0' && g_localSteering.m_keyCode <= '9') {
			g_menuKey = g_localSteering.m_keyCode;
			g_localSteering.m_keyCode = 0;
		}
		else {
			if (g_localSteering.m_keyCode == 0x1b) {
				g_localSteering.m_keyCode = 0;
			}
			else if (flags & 1) {
				switch (g_localSteering.m_keyCode) {
				case 0x09:
				case 0x0d:
				case 0x20:
				case 0xc6:
				case 0xc7:
				case 0xc8:
				case 0xc9:
				case 0x209:
					g_localSteering.m_keyCode = 0;
					break;
				default:
					break;
				}
			}

			if (bound == 0x1b || (bound && (flags & 1))) {
				g_menuKey = bound;
			}
			else if (flags & 1) {
				if (g_sinkMenuEnter) {
					g_menuKey = 0x0d;
				}
				else if (g_sinkMenuAbort) {
					g_menuKey = 0x1b;
				}

				if (g_menuKey == 0 && GetTicks(g_menuRepeatTimer) > 90) {
					if (g_sinkMenuItem < -0x2000) {
						g_menuKey = 0xc6;
					}
					else if (g_sinkMenuItem > 0x2000) {
						g_menuKey = 0xc7;
					}
				}

				if (g_menuKey == 0 && GetTicks(g_menuRepeatTimer) > 45) {
					if (g_sinkMenuValue < -0x2000) {
						g_menuKey = 0xc9;
					}
					else if (g_sinkMenuValue > 0x2000) {
						g_menuKey = 0x20;
					}
				}
			}
		}

		if (g_menuKey && (flags & 1)) {
			ResetTicks(g_menuRepeatTimer);
			g_sinkMenuItemReset = 1;
			g_sinkMenuValueReset = 1;
		}
	}
}

// Opens and closes in-mission menus as requested, then draws and runs the open one. Called from
// SimMain after UpdateMenuKey and HandleGameKeys, so a menu closed this frame still counts as open
// (g_openMenuCount) when that frame's game keys run.
void UpdateMenus(void)
{
	int32_t requested;
	int32_t state;
	MenuSlot* slot;
	int32_t done;

	slot = g_menuSlots;
	done = 0;
	while (slot && !done) {
		state = slot->m_state;
		requested = slot->m_requested;
		if (state != requested) {
			if (requested == 1) {
				if (ActivateMenu(slot)) {
					state = 1;
				}
			}
			else if (requested == 0) {
				DeactivateMenu(slot);
				state = 0;
			}
		}

		slot->m_state = state;
		slot->m_requested = requested;
		if (state == 1) {
			if (!(slot->m_definition->m_flags & 2) || !g_stretchPending) {
				if (!DrawAndRunMenu(slot->m_definition)) {
					slot->m_requested = 0;
				}
			}
			done = 1;
		}

		slot = slot->m_next;
	}
}

// Draws the open in-mission menu and acts on g_menuKey. Returns FALSE when the menu should close.
int32_t DrawAndRunMenu(MenuDefinition* p_menu)
{
	int32_t result;
	PANE* target;
	PANE* backgroundTarget;

	result = FALSE;
	target = p_menu->m_target;
	if (target == NULL) {
		return result;
	}

	backgroundTarget = p_menu->m_backgroundTarget;
	if (backgroundTarget == NULL) {
		return result;
	}

	LoadMenuResources(p_menu);
	if (p_menu->m_flags & 0x20) {
		VFX_pane_wipe(target, 0);
	}

	if (p_menu->m_background) {
		VFX_shape_draw(backgroundTarget, p_menu->m_background, 0, 0, 0);
	}

	if (p_menu->m_flags & 4) {
		OutlinePane(target, 1);
	}

	RunMenuItems(p_menu);
	if (!IsMenuPageStackEmpty(p_menu)) {
		result = TRUE;
	}
	else {
		result = FALSE;
	}

	return result;
}

// Acts on a menu key for the selected item: Enter depends on the item's type (0 opens a submenu,
// action 3; 2 goes back, action 4; 5 and 6 close, action 5); Escape closes (action 5); Down (0xc7)
// and Tab move the selection +1 through *p_move; Up (0xc6) and Shift+Tab (0x209) move it -1.
void ApplyMenuKey(int32_t p_key, int32_t p_itemType, int32_t* p_action, int32_t* p_move)
{
	switch (p_key) {
	case 0x09:
	case 0xc7:
		*p_move = 1;
		break;
	case 0xc6:
	case 0x209:
		*p_move = -1;
		break;
	case 0x0d:
		switch (p_itemType) {
		case 0:
			*p_action = 3;
			break;
		case 2:
			*p_action = 4;
			break;
		case 5:
		case 6:
			*p_action = 5;
			break;
		default:
			break;
		}
		break;
	case 0x1b:
		*p_action = 5;
		break;
	default:
		break;
	}
}

// Moves the open page's selection by g_menuKey (digits pick an item and act as Enter; the rest go
// through ApplyMenuKey, which skips headings), then draws the title and the items, numbered from 1
// with the accepting or closing item as 0, calling each item's m_run, which sees g_menuKey too.
// Finally opens the selected item's subpage (state 3) or closes the page (states 4 and 5).
void RunMenuItems(MenuDefinition* p_menu)
{
	int32_t back;
	char number[40];
	Point cursor;
	int32_t numberWidth;
	MenuPage* page;
	int32_t selected;
	Point textPos;
	int32_t offset;
	int32_t i;
	PANE* target;
	void* font;
	int32_t move;
	int32_t key;
	int32_t hasHeading;
	int32_t height;
	MenuItem* item;
	int32_t heading;
	int32_t n;
	Point controlPos;

	heading = 0x10;
	hasHeading = 0;
	move = 0;
	if (!p_menu) {
		return;
	}

	font = p_menu->m_font;
	if (!font) {
		return;
	}

	page = PeekMenuPage(p_menu);
	if (!page) {
		return;
	}

	target = p_menu->m_target;
	if (!target) {
		return;
	}

	selected = page->m_selected;
	back = -1;
	for (i = 0; i < page->m_itemCount; i++) {
		if (page->m_items[i].m_type == 2 || page->m_items[i].m_type == 6) {
			back = i;
		}

		if (page->m_items[i].m_type == 3) {
			heading = i;
			hasHeading = 1;
		}
	}

	if (back == -1) {
		hasHeading = 0;
	}

	if (page->m_state == 0) {
		page->m_state = 1;
	}
	else {
		if (page->m_state == 1) {
			page->m_state = 2;
		}

		if (g_menuKey) {
			key = g_menuKey;
			if (g_menuKey >= '0' && g_menuKey <= '9') {
				n = g_menuKey - '0';
				if (n == 0) {
					if (back != -1) {
						selected = back;
						key = '\r';
					}
				}
				else if (n >= 1 && page->m_itemCount - hasHeading > n) {
					if (n <= heading) {
						selected = n - 1;
					}
					else {
						selected = n;
						g_menuKey++;
						if (g_menuKey > '9') {
							g_menuKey = '0';
						}
					}

					key = '\r';
				}
			}

			ApplyMenuKey(key, page->m_items[selected].m_type, &page->m_state, &move);
			if (move) {
				selected = (page->m_itemCount + selected + move) % page->m_itemCount;
			}

			if (page->m_selected != selected && p_menu->m_moveSound != -1) {
				PlaySoundAt(0, 0, 0, p_menu->m_moveSound, 0);
			}
		}
	}

	if (move == 0) {
		move = 1;
	}

	if (page->m_items[selected].m_type == 3) {
		selected = (page->m_itemCount + selected + move) % page->m_itemCount;
		if (g_menuKey >= '0' && g_menuKey <= '9') {
			g_menuKey++;
			if (g_menuKey > '9') {
				g_menuKey = '0';
			}

			ApplyMenuKey(key, page->m_items[selected].m_type, &page->m_state, &move);
			if (move == 0) {
				move = 1;
			}
		}
	}

	page->m_selected = selected;
	g_textColors[0xe] = p_menu->m_color;
	if (page->m_title) {
		textPos = p_menu->m_titleOrigin;
		VFX_string_draw(target, textPos.m_x, textPos.m_y, font, page->m_title, g_textColors);
	}

	if (p_menu->m_flags & 4) {
		DrawRuleUnderRow(target, textPos, font, 1);
	}
	else {
		UnderlineText(target, page->m_title, textPos, font, 1);
	}

	height = VFX_font_height(font);
	cursor = p_menu->m_cursorOrigin;
	cursor.m_y += height / 2;
	textPos = p_menu->m_itemOrigin;
	controlPos = p_menu->m_controlOrigin;
	numberWidth = VFX_character_width(font, '0') * 2;
	numberWidth += VFX_character_width(font, '.');
	n = 0;
	offset = 0;
	for (i = 0; i < page->m_itemCount; i++) {
		if (selected == i) {
			g_textColors[0xe] = p_menu->m_highlightColor;
		}
		else {
			g_textColors[0xe] = p_menu->m_color;
		}

		item = &page->m_items[i];
		if (!(p_menu->m_flags & 8) && back == i) {
			offset += (p_menu->m_lineCount - i - 1) * p_menu->m_textOrigin.m_y;
		}

		textPos.m_y += offset;
		controlPos.m_y += offset;
		cursor.m_y += offset;
		offset = p_menu->m_textOrigin.m_y;
		if (selected == i && p_menu->m_cursorShape) {
			VFX_shape_draw(target, p_menu->m_cursorShape, 0, cursor.m_x, cursor.m_y);
		}

		if (item->m_type != 3) {
			if (back == i) {
				n = 0;
			}
			else {
				n++;
			}

			sprintf(number, "%d", n);
			VFX_string_draw(target, textPos.m_x, textPos.m_y, font, number, g_textColors);
		}

		if (item->m_text) {
			VFX_string_draw(target, textPos.m_x + numberWidth, textPos.m_y, font, item->m_text, g_textColors);
		}

		if (item->m_run) {
			item->m_run(p_menu, item->m_control, i, controlPos, page);
		}
	}

	g_textColors[0xe] = 0xe;
	switch (page->m_state) {
	case 4:
	case 5:
		page->m_state = 0;
		page->m_selected = 0;
		PopMenuPage(p_menu);
		break;
	case 3:
		page->m_state = 2;
		if (page->m_items[selected].m_subpage) {
			PushMenuPage(p_menu, page->m_items[selected].m_subpage);
			if (p_menu->m_openSound != -1) {
				PlaySoundAt(0, 0, 0, p_menu->m_openSound, 0);
			}
		}
		break;
	default:
		break;
	}
}

// Finds the g_menuSlots entry for a menu ID, or NULL.
MenuSlot* FindMenuSlot(int32_t p_id)
{
	MenuSlot* slot;

	slot = g_menuSlots;
	while (slot) {
		if (slot->m_id == p_id) {
			break;
		}
		slot = slot->m_next;
	}

	return slot;
}

// Returns the definition of an open menu, or NULL.
MenuDefinition* FindMenuDefinition(int32_t p_id)
{
	MenuSlot* slot;
	MenuDefinition* result;

	result = NULL;
	slot = g_menuSlots;
	while (slot) {
		if (slot->m_id == p_id) {
			break;
		}
		slot = slot->m_next;
	}

	if (slot) {
		result = slot->m_definition;
	}

	return result;
}

MenuPage* PopMenuPage(MenuDefinition* p_menu)
{
	MenuPage* page;

	page = NULL;
	if (p_menu->m_pageDepth > 0) {
		p_menu->m_pageDepth--;
		page = p_menu->m_pageStack[p_menu->m_pageDepth];
	}

	return page;
}

MenuPage* PeekMenuPage(MenuDefinition* p_menu)
{
	MenuPage* page;

	page = NULL;
	if (p_menu->m_pageDepth > 0) {
		page = p_menu->m_pageStack[p_menu->m_pageDepth - 1];
	}

	return page;
}

int32_t PushMenuPage(MenuDefinition* p_menu, MenuPage* p_page)
{
	int32_t result;

	result = 0;
	if (p_menu->m_pageDepth < 8) {
		p_menu->m_pageStack[p_menu->m_pageDepth] = p_page;
		p_menu->m_pageDepth++;
		result = 1;
	}

	return result;
}

void ClearMenuPages(MenuDefinition* p_menu)
{
	p_menu->m_pageDepth = 0;
}

int32_t IsMenuPageStackEmpty(MenuDefinition* p_menu)
{
	return p_menu->m_pageDepth == 0;
}

// Returns the open in-mission menu's definition, or NULL if none is open.
MenuDefinition* GetOpenMenu(void)
{
	MenuSlot* slot;
	MenuDefinition* result;

	slot = g_menuSlots;
	result = NULL;
	while (slot) {
		if (slot->m_state == 1) {
			result = slot->m_definition;
			break;
		}
		slot = slot->m_next;
	}

	return result;
}

// Returns a menu's state (1: open), or 0 if it isn't registered.
int32_t GetMenuSlotState(int32_t p_id)
{
	MenuSlot* slot;
	int32_t result;

	result = 0;
	slot = FindMenuSlot(p_id);
	if (slot) {
		result = slot->m_state;
	}

	return result;
}
