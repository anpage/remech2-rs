#ifndef MENUSCREEN_H
#define MENUSCREEN_H

#include "decomp.h"
#include "mainmenubutton.h"
#include "types.h"

// SIZE 0x10
// A menu screen of one campaign: its buttons, its background picture and its song. The song is
// -1 on the screens without one of their own, and matches PlayMidiSong's entry for the screen's
// message (0x24 to 0x29 for the clan halls, ready rooms and cadet training), but PlayMidiSong
// goes by the message and nothing reads m_song.
struct MenuScreen {
	MainMenuButton* m_buttons; // 0x00
	MechS32 m_count;           // 0x04
	MechS32 m_picture;         // 0x08 — for VideoDriver::LoadBackground
	MechS32 m_song;            // 0x0c
};

#endif // MENUSCREEN_H
