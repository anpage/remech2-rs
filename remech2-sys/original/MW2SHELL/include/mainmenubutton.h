#ifndef MAINMENUBUTTON_H
#define MAINMENUBUTTON_H

#include "decomp.h"
#include "types.h"

// SIZE 0x1c
// A button of a menu screen: its hit rectangle and a label. A label starting with '<' is drawn
// all the time, the others only while the mouse is over the button.
struct MainMenuButton {
	MechS32 m_left;   // 0x00
	MechS32 m_top;    // 0x04
	MechS32 m_right;  // 0x08
	MechS32 m_bottom; // 0x0c
	MechPoint m_textPos;  // 0x10
	MechChar* m_text; // 0x18
};

#endif // MAINMENUBUTTON_H
