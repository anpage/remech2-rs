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

	extern MechChar g_combatVariablesItem[17];
	extern MenuPage g_combatVariablesPage;
	extern MechChar g_combatVariablesTitle[17];
	extern MechChar g_objectTextmapsItem[16];
	extern MechChar g_terrainTextmapsItem[17];
	extern MechChar g_displayDetailItem[15];
	extern MechChar g_objectDensityItem[15];
	extern MechChar g_explosionChunksItem[17];
	extern MechChar g_affineText[7];
	extern MechChar g_perspectiveText[12];
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
