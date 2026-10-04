/* The in-mission menu's audio page: the music, sound effects and voice volumes. A data-only
   object: its data follows weaponpanel.c's. */
#include "audiomenu.h"

#include "audio.h"
#include "mainmenu.h"
#include "menu.h"
#include "menucontrol.h"
#include "menucontrols.h"
#include "menupage.h"
#include "types.h"

#include <stddef.h>

// GLOBAL: MW2 0x100a5288
MechChar g_audioItem[] = "Audio Ctrl";

// GLOBAL: MW2 0x100a5298
MechChar g_audioTitle[] = "SET AUDIO VOLUME";

// GLOBAL: MW2 0x100a52b0
MechChar g_bettyMessageItem[] = "Betty Message";

// GLOBAL: MW2 0x100a52c0
MechChar g_soundEffectsItem[] = "Sound Effects";

// GLOBAL: MW2 0x100a52d0
MechChar g_voiceItem[] = "Voice";

// GLOBAL: MW2 0x100a52d8
MechChar g_musicItem[] = "Music";

// GLOBAL: MW2 0x100a52e0
MenuControl g_soundEffectsControl =
	{2, 0, g_sliderShapes, 0, NULL, GetSoundSetting, PreviewSoundSetting, SetSoundSetting, RestoreSoundSetting};

// GLOBAL: MW2 0x100a5308
MenuControl g_voiceControl =
	{2, 0, g_sliderShapes, 1, NULL, GetSoundSetting, PreviewSoundSetting, SetSoundSetting, RestoreSoundSetting};

// GLOBAL: MW2 0x100a5330
MenuControl g_musicControl =
	{2, 0, g_sliderShapes, 2, NULL, GetSoundSetting, PreviewSoundSetting, SetSoundSetting, RestoreSoundSetting};

// GLOBAL: MW2 0x100a5358
MenuPage g_audioPage = {
	0,
	g_audioTitle,
	0,
	4,
	0,
	NULL,
	{{1, g_musicItem, RunMenuSlider, &g_musicControl, NULL},
	 {1, g_soundEffectsItem, RunMenuSlider, &g_soundEffectsControl, NULL},
	 {1, g_voiceItem, RunMenuSlider, &g_voiceControl, NULL},
	 {2, g_acceptText, NULL, NULL, NULL}}
};
