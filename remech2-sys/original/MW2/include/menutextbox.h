#ifndef MENUTEXTBOX_H
#define MENUTEXTBOX_H

#include "types.h"

struct PANE;

// The data of a text box control (MenuControl::m_data): a rectangle, in 16.16 fractions of the
// menu's target until first drawn, and its text.
typedef struct MenuTextBox {
	struct PANE* m_target;
	char* m_text;
} MenuTextBox;

#endif // MENUTEXTBOX_H
