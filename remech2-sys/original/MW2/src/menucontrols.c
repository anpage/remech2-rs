/* The in-mission menus' controls: the item callbacks that draw a slider, a list of choices, a
   list with a suffix and a text box, and edit their values while the item is selected. */
#include "menucontrols.h"

#include "fixedmul.h"
#include "loadres.h"
#include "menu.h"
#include "menuchoices.h"
#include "menutextbox.h"
#include "mw2prj.h"
#include "render.h"
#include "screenscale.h"
#include "setres.h"
#include "targeting.h"
#include "types.h"
#include "vfxa.h"

#include <string.h>

// A slider from 0 to 0x10000 (MenuControl::m_data holds its shapes: the left cap, the bar, the
// right cap and the knob): while selected, Space or Right (0xc8) raises it by a tenth and Left
// (0xc9) lowers it, and its number key raises it too.
void RunMenuSlider(MenuDefinition* p_menu, MenuControl* p_control, int32_t p_index, Point p_pos, MenuPage* p_page)
{
	int32_t pressed;
	int32_t step;
	int32_t* shapes;
	int32_t selected;
	uint32_t state;
	int32_t height;
	int32_t knobX;
	int32_t size;
	int32_t x;
	void* font;
	PANE* target;
	int32_t apply;
	int32_t width;
	void* knob;
	void* bar;
	int32_t value;
	void* right;
	void* left;

	step = 0;
	apply = true;
	if (!p_menu) {
		return;
	}

	font = p_menu->m_font;
	if (!font) {
		return;
	}

	if (!p_page) {
		return;
	}

	state = p_page->m_state;
	target = p_menu->m_target;
	if (!target) {
		return;
	}

	if (!p_control) {
		return;
	}

	shapes = p_control->m_data;
	if (!shapes) {
		return;
	}

	if (g_menuKey - '1' == p_index) {
		pressed = true;
	}
	else {
		pressed = false;
	}

	if ((p_control->m_flags & 1) && p_control->m_get) {
		value = p_control->m_get(p_control->m_arg);
	}
	else {
		value = p_control->m_value;
	}

	if (p_page->m_selected == p_index) {
		selected = true;
	}
	else {
		selected = false;
	}

	switch (state) {
	case 1:
		if (p_control->m_init) {
			p_control->m_init(p_page, p_control);
		}

		if (p_control->m_get) {
			p_control->m_value = p_control->m_get(p_control->m_arg);
			value = p_control->m_value;
		}
		else {
			p_control->m_value = 0;
			value = p_control->m_value;
		}
		break;
	case 2:
		if (selected) {
			switch (g_menuKey) {
			case ' ':
			case 0xc8:
				step = 0x199a;
				break;
			case 0xc9:
				step = -0x199a;
				break;
			default:
				if (pressed) {
					step = 0x199a;
				}
				break;
			}
		}
		break;
	case 4:
		if (p_control->m_set) {
			p_control->m_set(p_control->m_arg, value);
		}

		apply = false;
		break;
	case 5:
		if (p_control->m_cancel) {
			p_control->m_cancel(p_control->m_arg);
		}

		apply = false;
		break;
	}

	left = LoadCachedResource(g_mw2PrjHandle, shapes[0] + g_artResolution, g_resourceTypeTags[c_resTagShp], 0);
	bar = LoadCachedResource(g_mw2PrjHandle, shapes[2] + g_artResolution, g_resourceTypeTags[c_resTagShp], 0);
	knob = LoadCachedResource(g_mw2PrjHandle, shapes[6] + g_artResolution, g_resourceTypeTags[c_resTagShp], 0);
	right = LoadCachedResource(g_mw2PrjHandle, shapes[4] + g_artResolution, g_resourceTypeTags[c_resTagShp], 0);
	if (bar && knob) {
		size = VFX_shape_bounds(bar, 0);
		width = size >> 16;
		if ((value += step) < 0) {
			value = 0;
		}

		if (value > 0x10000) {
			value = 0x10000;
		}

		knobX = FixedMul16(value, width);
		height = VFX_font_height(font);
		p_pos.m_y += height / 2;
		x = p_pos.m_x;
		if (left) {
			VFX_shape_draw(target, left, 0, x, p_pos.m_y);
			x += VFX_shape_bounds(left, 0) >> 16;
		}

		VFX_shape_draw(target, bar, 0, x, p_pos.m_y);
		if (right) {
			VFX_shape_draw(target, right, 0, x + width, p_pos.m_y);
		}

		x += knobX;
		x -= (VFX_shape_bounds(knob, 0) >> 16) / 2;
		VFX_shape_draw(target, knob, 0, x, p_pos.m_y);
		if (selected) {
			if (((step && p_page->m_items[p_index].m_type == 4) || (pressed && p_page->m_items[p_index].m_type == 5)) &&
				p_control->m_set) {
				p_control->m_set(p_control->m_arg, value);
			}

			if (p_control->m_preview && apply) {
				p_control->m_preview(p_control->m_arg, value);
			}
		}
	}

	p_control->m_value = value;
	UnlockCachedResource(shapes[0] + g_artResolution, g_resourceTypeTags[c_resTagShp]);
	UnlockCachedResource(shapes[2] + g_artResolution, g_resourceTypeTags[c_resTagShp]);
	UnlockCachedResource(shapes[4] + g_artResolution, g_resourceTypeTags[c_resTagShp]);
	UnlockCachedResource(shapes[6] + g_artResolution, g_resourceTypeTags[c_resTagShp]);
}

// A list of choices shown with a suffix from MenuChoices::m_suffix; it changes only through its
// callbacks.
void RunMenuStatus(MenuDefinition* p_menu, MenuControl* p_control, int32_t p_index, Point p_pos, MenuPage* p_page)
{
	int32_t pressed;
	int32_t value;
	char* suffix;
	int32_t selected;
	uint32_t state;
	int32_t room;
	int32_t step;
	PANE* target;
	int32_t apply;
	char text[128];

	step = 0;
	apply = true;
	if (!p_menu) {
		return;
	}

	if (!p_menu->m_font) {
		return;
	}

	if (!p_page) {
		return;
	}

	state = p_page->m_state;
	target = p_menu->m_target;
	if (!target) {
		return;
	}

	if (!p_control) {
		return;
	}

	if (!p_control->m_data) {
		return;
	}

	if (g_menuKey - '1' == p_index) {
		pressed = true;
	}
	else {
		pressed = false;
	}

	if ((p_control->m_flags & 1) && p_control->m_get) {
		value = p_control->m_get(p_control->m_arg);
	}
	else {
		value = p_control->m_value;
	}

	if (p_page->m_selected == p_index) {
		selected = true;
	}
	else {
		selected = false;
	}

	switch (state) {
	case 1:
		if (p_control->m_init) {
			p_control->m_init(p_page, p_control);
		}

		if (p_control->m_get) {
			p_control->m_value = p_control->m_get(p_control->m_arg);
			value = p_control->m_value;
		}
		else {
			p_control->m_value = 0;
			value = p_control->m_value;
		}
		break;
	case 2:
		break;
	case 4:
		if (p_control->m_set) {
			p_control->m_set(p_control->m_arg, value);
		}

		apply = false;
		break;
	case 5:
		if (p_control->m_cancel) {
			p_control->m_cancel(p_control->m_arg);
		}

		apply = false;
		break;
	case 3:
		break;
	}

	if (((MenuChoices*) p_control->m_data)->m_count > 0) {
		value =
			(((MenuChoices*) p_control->m_data)->m_count + value + step) % ((MenuChoices*) p_control->m_data)->m_count;
		strncpy(text, ((MenuChoices*) p_control->m_data)->m_texts[value], 127);
		text[127] = '\0';
		if (((MenuChoices*) p_control->m_data)->m_suffix) {
			suffix = ((MenuChoices*) p_control->m_data)->m_suffix(p_menu, p_control, p_index, p_pos, p_page);
			if (suffix) {
				room = 127 - strlen(text);
				strncat(text, suffix, room);
				text[127] = '\0';
			}
		}

		VFX_string_draw(target, p_pos.m_x, p_pos.m_y, p_menu->m_font, text, g_textColors);
	}

	if (selected) {
		if (pressed && p_page->m_items[p_index].m_type == 5 && p_control->m_set) {
			p_control->m_set(p_control->m_arg, value);
		}

		if (p_control->m_preview && apply) {
			p_control->m_preview(p_control->m_arg, value);
		}
	}

	p_control->m_value = value;
}

// A list of choices (MenuChoices): while selected, Space or Right (0xc8) picks the next and Left
// (0xc9) the previous, wrapping, and its number key the next.
void RunMenuChoice(MenuDefinition* p_menu, MenuControl* p_control, int32_t p_index, Point p_pos, MenuPage* p_page)
{
	int32_t pressed;
	int32_t value;
	int32_t selected;
	uint32_t state;
	int32_t step;
	PANE* target;
	int32_t apply;

	step = 0;
	apply = true;
	if (!p_menu) {
		return;
	}

	if (!p_menu->m_font) {
		return;
	}

	if (!p_page) {
		return;
	}

	state = p_page->m_state;
	target = p_menu->m_target;
	if (!target) {
		return;
	}

	if (!p_control) {
		return;
	}

	if (!p_control->m_data) {
		return;
	}

	if (g_menuKey - '1' == p_index) {
		pressed = true;
	}
	else {
		pressed = false;
	}

	if ((p_control->m_flags & 1) && p_control->m_get) {
		value = p_control->m_get(p_control->m_arg);
	}
	else {
		value = p_control->m_value;
	}

	if (p_page->m_selected == p_index) {
		selected = true;
	}
	else {
		selected = false;
	}

	switch (state) {
	case 1:
		if (p_control->m_init) {
			p_control->m_init(p_page, p_control);
		}

		if (p_control->m_get) {
			p_control->m_value = p_control->m_get(p_control->m_arg);
			value = p_control->m_value;
		}
		else {
			p_control->m_value = 0;
			value = p_control->m_value;
		}
		break;
	case 2:
		if (selected) {
			switch (g_menuKey) {
			case ' ':
			case 0xc8:
				step = 1;
				break;
			case 0xc9:
				step = -1;
				break;
			default:
				if (pressed) {
					step = 1;
				}
				break;
			}
		}
		break;
	case 4:
		if (p_control->m_set) {
			p_control->m_set(p_control->m_arg, value);
		}

		apply = false;
		break;
	case 5:
		if (p_control->m_cancel) {
			p_control->m_cancel(p_control->m_arg);
		}

		apply = false;
		break;
	}

	if (((MenuChoices*) p_control->m_data)->m_count > 0) {
		value =
			(((MenuChoices*) p_control->m_data)->m_count + step + value) % ((MenuChoices*) p_control->m_data)->m_count;
		VFX_string_draw(
			target,
			p_pos.m_x,
			p_pos.m_y,
			p_menu->m_font,
			((MenuChoices*) p_control->m_data)->m_texts[value],
			g_textColors
		);
	}

	if (selected) {
		if (((step && p_page->m_items[p_index].m_type == 4) || (pressed && p_page->m_items[p_index].m_type == 5)) &&
			p_control->m_set) {
			p_control->m_set(p_control->m_arg, value);
		}

		if (p_control->m_preview && apply) {
			p_control->m_preview(p_control->m_arg, value);
		}
	}

	p_control->m_value = value;
}

// A text box (MenuTextBox) in the menu's font; its rectangle is placed in the menu's target the
// first time. Reports 0 through m_preview unless the menu is being accepted or cancelled.
void RunMenuTextBox(MenuDefinition* p_menu, MenuControl* p_control, int32_t p_index, Point p_pos, MenuPage* p_page)
{
	MenuTextBox* box;
	uint32_t state;
	char* text;
	void* font;
	PANE* rect;
	PANE* target;

	if (!p_menu) {
		return;
	}

	if (!p_page) {
		return;
	}

	state = p_page->m_state;
	target = p_menu->m_target;
	if (!target) {
		return;
	}

	font = p_menu->m_font;
	if (!font) {
		return;
	}

	if (!p_control) {
		return;
	}

	box = p_control->m_data;
	if (!box) {
		return;
	}

	rect = box->m_target;
	if (!rect) {
		return;
	}

	text = box->m_text;
	if (!text) {
		return;
	}

	if (!rect->m_window) {
		rect->m_window = &g_mainPixelBuffer;
		ScaleRectToFrame(target, rect, rect);
	}

	DrawWrappedText(rect, text, font);
	if (p_control->m_preview && state != 4 && state != 5) {
		p_control->m_preview(p_control->m_arg, 0);
	}
}
