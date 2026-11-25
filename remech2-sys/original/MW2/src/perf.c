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

int32_t ApplyPerfSettings(MenuDefinition* p_menu, MenuPage* p_page);
int32_t GetObjectTextmaps(int32_t p_arg);
int32_t GetTerrainTextmaps(int32_t p_arg);
int32_t GetDisplayDetail(int32_t p_arg);
int32_t GetObjectDensity(int32_t p_arg);
void SetObjectTextmaps(int32_t p_arg, int32_t p_objectTextmaps);
void SetTerrainTextmaps(int32_t p_arg, int32_t p_terrainTextmaps);
void SetDisplayDetail(int32_t p_arg, int32_t p_displayDetail);
void SetObjectDensity(int32_t p_arg, int32_t p_objectDensity);

char g_combatVariablesItem[] = "Combat Variables";

char g_combatVariablesTitle[] = "COMBAT VARIABLES";

char g_objectTextmapsItem[] = "Object textmaps";

char g_terrainTextmapsItem[] = "Terrain textmaps";

char g_displayDetailItem[] = "Display detail";

char g_objectDensityItem[] = "Object density";

char g_explosionChunksItem[] = "Explosion chunks";

char g_affineText[] = "Affine";

char g_perspectiveText[] = "Perspective";

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

int32_t ApplyPerfSettings(MenuDefinition* p_menu, MenuPage* p_page)
{
	int32_t result = 1;

	if (g_mw2SndCfgData != NULL) {
		SetObjectTextmaps(result, g_mw2SndCfgData->m_objectTextmaps);
		SetTerrainTextmaps(result, g_mw2SndCfgData->m_terrainTextmaps);
		SetDisplayDetail(result, g_mw2SndCfgData->m_displayDetail);
		SetObjectDensity(result, g_mw2SndCfgData->m_objectDensity);
		SetExplosionChunks(result, g_mw2SndCfgData->m_explosionChunks);
	}

	return result;
}

int32_t GetObjectTextmaps(int32_t p_arg)
{
	return AreTextureMapsOn(0x100) || AreTextureMapsOn(0x200);
}

void SetObjectTextmaps(int32_t p_arg, int32_t p_objectTextmaps)
{
	EnableTextureMaps(0x100, p_objectTextmaps);
	EnableTextureMaps(0x200, p_objectTextmaps);
	g_mw2SndCfgData->m_objectTextmaps = p_objectTextmaps;
}

int32_t GetTerrainTextmaps(int32_t p_arg)
{
	return AreTextureMapsOn(0x800) || g_gridObjectShown;
}

void SetTerrainTextmaps(int32_t p_arg, int32_t p_terrainTextmaps)
{
	EnableTextureMaps(0x800, p_terrainTextmaps);
	ShowGridObject(p_terrainTextmaps);
	g_mw2SndCfgData->m_terrainTextmaps = p_terrainTextmaps;
}

int32_t GetDisplayDetail(int32_t p_arg)
{
	return IsLodQualityHigh(p_arg) || ArePerspectiveTexturesOn(p_arg);
}

void SetDisplayDetail(int32_t p_arg, int32_t p_displayDetail)
{
	SetLodQualityHigh(p_arg, p_displayDetail);
	EnablePerspectiveTextures(p_arg, p_displayDetail);
	g_mw2SndCfgData->m_displayDetail = p_displayDetail;
}

int32_t GetObjectDensity(int32_t p_arg)
{
	return g_mw2SndCfgData->m_objectDensity;
}

void SetObjectDensity(int32_t p_arg, int32_t p_objectDensity)
{
	ShowDensityShapes(p_objectDensity);
	g_mw2SndCfgData->m_objectDensity = p_objectDensity;
}
