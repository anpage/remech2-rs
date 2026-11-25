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

char g_audioItem[] = "Audio Ctrl";

char g_audioTitle[] = "SET AUDIO VOLUME";

char g_bettyMessageItem[] = "Betty Message";

char g_soundEffectsItem[] = "Sound Effects";

char g_voiceItem[] = "Voice";

char g_musicItem[] = "Music";

MenuControl g_soundEffectsControl =
	{2, 0, g_sliderShapes, 0, NULL, GetSoundSetting, PreviewSoundSetting, SetSoundSetting, RestoreSoundSetting};

MenuControl g_voiceControl =
	{2, 0, g_sliderShapes, 1, NULL, GetSoundSetting, PreviewSoundSetting, SetSoundSetting, RestoreSoundSetting};

MenuControl g_musicControl =
	{2, 0, g_sliderShapes, 2, NULL, GetSoundSetting, PreviewSoundSetting, SetSoundSetting, RestoreSoundSetting};

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
