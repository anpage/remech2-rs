#include "perf.h"

#include "audio.h"
#include "decomp.h"
#include "faceshade.h"
#include "geocache.h"
#include "gridobject.h"
#include "mainmenu.h"
#include "menu.h"
#include "menucontrol.h"
#include "menucontrols.h"
#include "menupage.h"
#include "shapelists.h"
#include "soundconfig.h"
#include "types.h"
#include "view.h"

// The player's display performance settings: a getter and a setter each,
// with the unused leading argument of the in-mission menu's callbacks.

MechS32 ApplyPerfSettings(MenuDefinition* p_menu, MenuPage* p_page);
MechS32 GetObjectTextmaps(MechS32 p_arg);
MechS32 GetTerrainTextmaps(MechS32 p_arg);
MechS32 GetDisplayDetail(MechS32 p_arg);
MechS32 GetObjectDensity(MechS32 p_arg);
void SetObjectTextmaps(MechS32 p_arg, MechS32 p_objectTextmaps);
void SetTerrainTextmaps(MechS32 p_arg, MechS32 p_terrainTextmaps);
void SetDisplayDetail(MechS32 p_arg, MechS32 p_displayDetail);
void SetObjectDensity(MechS32 p_arg, MechS32 p_objectDensity);

// GLOBAL: MW2 0x100b1438
MechChar g_combatVariablesItem[] = "Combat Variables";

// GLOBAL: MW2 0x100b1450
MechChar g_combatVariablesTitle[] = "COMBAT VARIABLES";

// GLOBAL: MW2 0x100b1468
MechChar g_objectTextmapsItem[] = "Object textmaps";

// GLOBAL: MW2 0x100b1478
MechChar g_terrainTextmapsItem[] = "Terrain textmaps";

// GLOBAL: MW2 0x100b1490
MechChar g_displayDetailItem[] = "Display detail";

// GLOBAL: MW2 0x100b14a0
MechChar g_objectDensityItem[] = "Object density";

// GLOBAL: MW2 0x100b14b0
MechChar g_explosionChunksItem[] = "Explosion chunks";

// GLOBAL: MW2 0x100b14c8
MechChar g_affineText[] = "Affine";

// GLOBAL: MW2 0x100b14d0
MechChar g_perspectiveText[] = "Perspective";

// GLOBAL: MW2 0x100b14e0
MenuChoices g_affinePerspectiveChoices = {NULL, 2, {g_affineText, g_perspectiveText}};

// GLOBAL: MW2 0x100b1528
MenuControl g_objectTextmapsControl =
	{2, 0, &g_offOnChoices, 0, NULL, GetObjectTextmaps, NULL, SetObjectTextmaps, NULL};

// GLOBAL: MW2 0x100b1550
MenuControl g_terrainTextmapsControl =
	{2, 0, &g_offOnChoices, 0, NULL, GetTerrainTextmaps, NULL, SetTerrainTextmaps, NULL};

// GLOBAL: MW2 0x100b1578
MenuControl g_displayDetailControl = {2, 0, &g_lowHighChoices, 0, NULL, GetDisplayDetail, NULL, SetDisplayDetail, NULL};

// GLOBAL: MW2 0x100b15a0
MenuControl g_objectDensityControl = {2, 0, &g_lowHighChoices, 0, NULL, GetObjectDensity, NULL, SetObjectDensity, NULL};

// GLOBAL: MW2 0x100b15c8
MenuControl g_explosionChunksControl =
	{2, 0, &g_offOnChoices, 0, NULL, GetExplosionChunks, NULL, SetExplosionChunks, NULL};

// GLOBAL: MW2 0x100b15f0
MenuPage g_combatVariablesPage = {
	0,
	g_combatVariablesTitle,
	0,
	6,
	0,
	ApplyPerfSettings,
	{{1, g_objectTextmapsItem, RunMenuChoice, &g_objectTextmapsControl, NULL},
	 {1, g_terrainTextmapsItem, RunMenuChoice, &g_terrainTextmapsControl, NULL},
	 {1, g_displayDetailItem, RunMenuChoice, &g_displayDetailControl, NULL},
	 {1, g_objectDensityItem, RunMenuChoice, &g_objectDensityControl, NULL},
	 {1, g_explosionChunksItem, RunMenuChoice, &g_explosionChunksControl, NULL},
	 {2, g_acceptText, NULL, NULL, NULL}}
};

// FUNCTION: MW2 0x10076af0
void FirstPerfSetting(void)
{
	ApplyPerfSettings(NULL, NULL);
}

// FUNCTION: MW2 0x10076b07
MechS32 ApplyPerfSettings(MenuDefinition* p_menu, MenuPage* p_page)
{
	MechS32 result = 1;

	if (g_mw2SndCfgData != NULL) {
		SetObjectTextmaps(result, g_mw2SndCfgData->m_objectTextmaps);
		SetTerrainTextmaps(result, g_mw2SndCfgData->m_terrainTextmaps);
		SetDisplayDetail(result, g_mw2SndCfgData->m_displayDetail);
		SetObjectDensity(result, g_mw2SndCfgData->m_objectDensity);
		SetExplosionChunks(result, g_mw2SndCfgData->m_explosionChunks);
	}

	return result;
}

// FUNCTION: MW2 0x10076b9a
MechS32 GetObjectTextmaps(MechS32 p_arg)
{
	return AreTextureMapsOn(0x100) || AreTextureMapsOn(0x200);
}

// FUNCTION: MW2 0x10076be0
void SetObjectTextmaps(MechS32 p_arg, MechS32 p_objectTextmaps)
{
	EnableTextureMaps(0x100, p_objectTextmaps);
	EnableTextureMaps(0x200, p_objectTextmaps);
	g_mw2SndCfgData->m_objectTextmaps = p_objectTextmaps;
}

// FUNCTION: MW2 0x10076c19
MechS32 GetTerrainTextmaps(MechS32 p_arg)
{
	return AreTextureMapsOn(0x800) || g_gridObjectShown;
}

// FUNCTION: MW2 0x10076c57
void SetTerrainTextmaps(MechS32 p_arg, MechS32 p_terrainTextmaps)
{
	EnableTextureMaps(0x800, p_terrainTextmaps);
	ShowGridObject(p_terrainTextmaps);
	g_mw2SndCfgData->m_terrainTextmaps = p_terrainTextmaps;
}

// FUNCTION: MW2 0x10076c8b
MechS32 GetDisplayDetail(MechS32 p_arg)
{
	return IsLodQualityHigh(p_arg) || ArePerspectiveTexturesOn(p_arg);
}

// FUNCTION: MW2 0x10076ccf
void SetDisplayDetail(MechS32 p_arg, MechS32 p_displayDetail)
{
	SetLodQualityHigh(p_arg, p_displayDetail);
	EnablePerspectiveTextures(p_arg, p_displayDetail);
	g_mw2SndCfgData->m_displayDetail = p_displayDetail;
}

// FUNCTION: MW2 0x10076d06
MechS32 GetObjectDensity(MechS32 p_arg)
{
	return g_mw2SndCfgData->m_objectDensity;
}

// FUNCTION: MW2 0x10076d1e
void SetObjectDensity(MechS32 p_arg, MechS32 p_objectDensity)
{
	ShowDensityShapes(p_objectDensity);
	g_mw2SndCfgData->m_objectDensity = p_objectDensity;
}
