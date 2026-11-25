#ifndef MENUSCREEN_H
#define MENUSCREEN_H

#include "mainmenubutton.h"
#include "types.h"

// A menu screen of one campaign: its buttons, its background picture and its song. The song is
// -1 on the screens without one of their own, and matches PlayMidiSong's entry for the screen's
// message (0x24 to 0x29 for the clan halls, ready rooms and cadet training), but PlayMidiSong
// goes by the message and nothing reads m_song.
struct MenuScreen {
	MainMenuButton* m_buttons; // 0x00
	int32_t m_count;           // 0x04
	int32_t m_picture;         // 0x08 — for VideoDriver::LoadBackground
	int32_t m_song;            // 0x0c
};

#endif // MENUSCREEN_H
