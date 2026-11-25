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

	extern char g_audioItem[11];
	extern MenuPage g_audioPage;
	extern char g_audioTitle[17];
	extern char g_bettyMessageItem[14];
	extern char g_soundEffectsItem[14];
	extern char g_voiceItem[6];
	extern char g_musicItem[6];
	extern MenuControl g_soundEffectsControl;
	extern MenuControl g_voiceControl;
	extern MenuControl g_musicControl;

#ifdef __cplusplus
}
#endif

#endif // AUDIOMENU_H
