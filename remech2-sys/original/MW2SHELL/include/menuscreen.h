#ifndef MENUSCREEN_H
#define MENUSCREEN_H

#include "mainmenubutton.h"
#include "types.h"

// A menu screen of one campaign: its buttons, its background picture and its song. The song is
// -1 on the screens without one of their own, and matches PlayMidiSong's entry for the screen's
// message (0x24 to 0x29 for the clan halls, ready rooms and cadet training), but PlayMidiSong
// goes by the message and nothing reads m_song.
struct MenuScreen {
	MainMenuButton* m_buttons;
	int32_t m_count;
	int32_t m_picture; // for VideoDriver::LoadBackground
	int32_t m_song;
};

#endif // MENUSCREEN_H
