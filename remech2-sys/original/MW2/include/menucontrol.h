#ifndef MENUCONTROL_H
#define MENUCONTROL_H

#include "decomp.h"
#include "types.h"

struct MenuPage;

// A value a menu item edits (menucontrols.c: a slider, a list of choices or a text box): the
// value, the control's own data and the callbacks that read and apply it (the brightness menu's
// are GetBrightnessFraction, PreviewBrightnessFraction, SetBrightnessFraction and
// RestoreBrightness).
// SIZE 0x28
typedef struct MenuControl {
	MechU32 m_flags; // 0x00 — 1: read the value through m_get every time
	MechS32 m_value; // 0x04
	void* m_data;    // 0x08 — shape ids, MenuChoices or a MenuTextBox
	MechS32 m_arg;   // 0x0c — passed to the callbacks
	void (*m_init)(struct MenuPage* p_page, struct MenuControl* p_control); // 0x10
	MechS32 (*m_get)(MechS32 p_arg);                                        // 0x14
	void (*m_preview)(MechS32 p_arg, MechS32 p_value);                      // 0x18 — when the value changes
	void (*m_set)(MechS32 p_arg, MechS32 p_value);                          // 0x1c — when the menu is accepted
	void (*m_cancel)(MechS32 p_arg);                                        // 0x20 — when the menu is cancelled
	undefined4 m_unk0x24;                                                   // 0x24
} MenuControl;

#endif // MENUCONTROL_H
