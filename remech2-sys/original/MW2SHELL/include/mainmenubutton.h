#ifndef MAINMENUBUTTON_H
#define MAINMENUBUTTON_H

#include "types.h"

// A button of a menu screen: its hit rectangle and a label. A label starting with '<' is drawn
// all the time, the others only while the mouse is over the button.
struct MainMenuButton {
	int32_t m_left;
	int32_t m_top;
	int32_t m_right;
	int32_t m_bottom;
	MechPoint m_textPos;
	char* m_text;
};

#endif // MAINMENUBUTTON_H
