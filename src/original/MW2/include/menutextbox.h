#ifndef MENUTEXTBOX_H
#define MENUTEXTBOX_H

#include "types.h"

struct PANE;

// The data of a text box control (MenuControl::m_data): a rectangle, in 16.16 fractions of the
// menu's target until first drawn, and its text.
// SIZE 0x08
typedef struct MenuTextBox {
	struct PANE* m_target; // 0x00
	MechChar* m_text;      // 0x04
} MenuTextBox;

#endif // MENUTEXTBOX_H
