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

MechChar g_combatVariablesItem[] = "Combat Variables";

MechChar g_combatVariablesTitle[] = "COMBAT VARIABLES";

MechChar g_objectTextmapsItem[] = "Object textmaps";

MechChar g_terrainTextmapsItem[] = "Terrain textmaps";

MechChar g_displayDetailItem[] = "Display detail";

MechChar g_objectDensityItem[] = "Object density";

MechChar g_explosionChunksItem[] = "Explosion chunks";

MechChar g_affineText[] = "Affine";

MechChar g_perspectiveText[] = "Perspective";

MenuChoices g_affinePerspectiveChoices = {NULL, 2, {g_affineText, g_perspectiveText}};

MenuControl g_objectTextmapsControl =
	{2, 0, &g_offOnChoices, 0, NULL, GetObjectTextmaps, NULL, SetObjectTextmaps, NULL};

MenuControl g_terrainTextmapsControl =
	{2, 0, &g_offOnChoices, 0, NULL, GetTerrainTextmaps, NULL, SetTerrainTextmaps, NULL};

MenuControl g_displayDetailControl = {2, 0, &g_lowHighChoices, 0, NULL, GetDisplayDetail, NULL, SetDisplayDetail, NULL};

MenuControl g_objectDensityControl = {2, 0, &g_lowHighChoices, 0, NULL, GetObjectDensity, NULL, SetObjectDensity, NULL};

MenuControl g_explosionChunksControl =
	{2, 0, &g_offOnChoices, 0, NULL, GetExplosionChunks, NULL, SetExplosionChunks, NULL};

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

void FirstPerfSetting(void)
{
	ApplyPerfSettings(NULL, NULL);
}

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

MechS32 GetObjectTextmaps(MechS32 p_arg)
{
	return AreTextureMapsOn(0x100) || AreTextureMapsOn(0x200);
}

void SetObjectTextmaps(MechS32 p_arg, MechS32 p_objectTextmaps)
{
	EnableTextureMaps(0x100, p_objectTextmaps);
	EnableTextureMaps(0x200, p_objectTextmaps);
	g_mw2SndCfgData->m_objectTextmaps = p_objectTextmaps;
}

MechS32 GetTerrainTextmaps(MechS32 p_arg)
{
	return AreTextureMapsOn(0x800) || g_gridObjectShown;
}

void SetTerrainTextmaps(MechS32 p_arg, MechS32 p_terrainTextmaps)
{
	EnableTextureMaps(0x800, p_terrainTextmaps);
	ShowGridObject(p_terrainTextmaps);
	g_mw2SndCfgData->m_terrainTextmaps = p_terrainTextmaps;
}

MechS32 GetDisplayDetail(MechS32 p_arg)
{
	return IsLodQualityHigh(p_arg) || ArePerspectiveTexturesOn(p_arg);
}

void SetDisplayDetail(MechS32 p_arg, MechS32 p_displayDetail)
{
	SetLodQualityHigh(p_arg, p_displayDetail);
	EnablePerspectiveTextures(p_arg, p_displayDetail);
	g_mw2SndCfgData->m_displayDetail = p_displayDetail;
}

MechS32 GetObjectDensity(MechS32 p_arg)
{
	return g_mw2SndCfgData->m_objectDensity;
}

void SetObjectDensity(MechS32 p_arg, MechS32 p_objectDensity)
{
	ShowDensityShapes(p_objectDensity);
	g_mw2SndCfgData->m_objectDensity = p_objectDensity;
}
