#ifndef MENUCHOICES_H
#define MENUCHOICES_H

#include "point.h"
#include "types.h"

struct MenuControl;
struct MenuDefinition;
struct MenuPage;

// The text a list control appends to its choice (RunMenuStatus).
typedef char* (*MenuChoicesSuffixFn)(
	struct MenuDefinition* p_menu,
	struct MenuControl* p_control,
	int32_t p_index,
	Point p_pos,
	struct MenuPage* p_page
);

// The data of a list control (MenuControl::m_data): the texts of its choices.
typedef struct MenuChoices {
	MenuChoicesSuffixFn m_suffix;
	int32_t m_count;
	char* m_texts[16]; // m_count of them
} MenuChoices;

#endif // MENUCHOICES_H
