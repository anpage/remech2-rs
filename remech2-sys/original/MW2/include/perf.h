#ifndef PERF_H
#define PERF_H

#include "menu.h"
#include "menuchoices.h"
#include "menucontrol.h"
#include "menupage.h"
#include "types.h"

// The functions and globals of perf.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern char g_combatVariablesItem[17];
	extern MenuPage g_combatVariablesPage;
	extern char g_combatVariablesTitle[17];
	extern char g_objectTextmapsItem[16];
	extern char g_terrainTextmapsItem[17];
	extern char g_displayDetailItem[15];
	extern char g_objectDensityItem[15];
	extern char g_explosionChunksItem[17];
	extern char g_affineText[7];
	extern char g_perspectiveText[12];
	extern MenuChoices g_affinePerspectiveChoices;
	extern MenuControl g_objectTextmapsControl;
	extern MenuControl g_terrainTextmapsControl;
	extern MenuControl g_displayDetailControl;
	extern MenuControl g_objectDensityControl;
	extern MenuControl g_explosionChunksControl;

	void FirstPerfSetting(void);

#ifdef __cplusplus
}
#endif

#endif // PERF_H
