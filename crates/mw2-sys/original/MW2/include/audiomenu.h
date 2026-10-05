#ifndef AUDIOMENU_H
#define AUDIOMENU_H

#include "menu.h"
#include "menucontrol.h"
#include "menupage.h"
#include "types.h"

// The globals of audiomenu.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechChar g_audioItem[11];
	extern MenuPage g_audioPage;
	extern MechChar g_audioTitle[17];
	extern MechChar g_bettyMessageItem[14];
	extern MechChar g_soundEffectsItem[14];
	extern MechChar g_voiceItem[6];
	extern MechChar g_musicItem[6];
	extern MenuControl g_soundEffectsControl;
	extern MenuControl g_voiceControl;
	extern MenuControl g_musicControl;

#ifdef __cplusplus
}
#endif

#endif // AUDIOMENU_H
