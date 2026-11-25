#ifndef MENUCONTROL_H
#define MENUCONTROL_H

#include "types.h"

struct MenuPage;

// A value a menu item edits (menucontrols.c: a slider, a list of choices or a text box): the
// value, the control's own data and the callbacks that read and apply it (the brightness menu's
// are GetBrightnessFraction, PreviewBrightnessFraction, SetBrightnessFraction and
// RestoreBrightness).
typedef struct MenuControl {
	uint32_t m_flags; // 1: read the value through m_get every time
	int32_t m_value;
	void* m_data;  // shape ids, MenuChoices or a MenuTextBox
	int32_t m_arg; // passed to the callbacks
	void (*m_init)(struct MenuPage* p_page, struct MenuControl* p_control);
	int32_t (*m_get)(int32_t p_arg);
	void (*m_preview)(int32_t p_arg, int32_t p_value); // when the value changes
	void (*m_set)(int32_t p_arg, int32_t p_value);     // when the menu is accepted
	void (*m_cancel)(int32_t p_arg);                   // when the menu is cancelled
} MenuControl;

#endif // MENUCONTROL_H
