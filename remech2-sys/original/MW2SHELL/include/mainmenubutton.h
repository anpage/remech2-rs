#ifndef MAINMENUBUTTON_H
#define MAINMENUBUTTON_H

#include "decomp.h"
#include "types.h"

// A button of a menu screen: its hit rectangle and a label. A label starting with '<' is drawn
// all the time, the others only while the mouse is over the button.
struct MainMenuButton {
	int32_t m_left;      // 0x00
	int32_t m_top;       // 0x04
	int32_t m_right;     // 0x08
	int32_t m_bottom;    // 0x0c
	MechPoint m_textPos; // 0x10
	char* m_text;        // 0x18
};

#endif // MAINMENUBUTTON_H
