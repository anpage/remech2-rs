#ifndef MENUCONTROL_H
#define MENUCONTROL_H

#include "types.h"

struct MenuPage;

// A value a menu item edits (menucontrols.c: a slider, a list of choices or a text box): the
// value, the control's own data and the callbacks that read and apply it (the brightness menu's
// are GetBrightnessFraction, PreviewBrightnessFraction, SetBrightnessFraction and
// RestoreBrightness).
typedef struct MenuControl {
	uint32_t m_flags; // 0x00 — 1: read the value through m_get every time
	int32_t m_value;  // 0x04
	void* m_data;     // 0x08 — shape ids, MenuChoices or a MenuTextBox
	int32_t m_arg;    // 0x0c — passed to the callbacks
	void (*m_init)(struct MenuPage* p_page, struct MenuControl* p_control); // 0x10
	int32_t (*m_get)(int32_t p_arg);                                        // 0x14
	void (*m_preview)(int32_t p_arg, int32_t p_value);                      // 0x18 — when the value changes
	void (*m_set)(int32_t p_arg, int32_t p_value);                          // 0x1c — when the menu is accepted
	void (*m_cancel)(int32_t p_arg);                                        // 0x20 — when the menu is cancelled
} MenuControl;

#endif // MENUCONTROL_H
