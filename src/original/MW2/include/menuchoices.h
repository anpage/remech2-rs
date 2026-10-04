#ifndef MENUCHOICES_H
#define MENUCHOICES_H

#include "point.h"
#include "types.h"

struct MenuControl;
struct MenuDefinition;
struct MenuPage;

// The text a list control appends to its choice (RunMenuStatus).
typedef MechChar* (*MenuChoicesSuffixFn)(
	struct MenuDefinition* p_menu,
	struct MenuControl* p_control,
	MechS32 p_index,
	Point p_pos,
	struct MenuPage* p_page
);

// The data of a list control (MenuControl::m_data): the texts of its choices.
// SIZE 0x48
typedef struct MenuChoices {
	MenuChoicesSuffixFn m_suffix; // 0x00
	MechS32 m_count;              // 0x04
	MechChar* m_texts[16];        // 0x08 — m_count of them
} MenuChoices;

#endif // MENUCHOICES_H
