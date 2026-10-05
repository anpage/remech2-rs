#include "mw2prj.h"

#include "ai.h"
#include "config.h"
#include "error.h"
#include "loadres.h"
#include "prjfile.h"
#include "simmain.h"
#include "types.h"
#include "weapons.h"

#include <string.h>

// The resource type tags in the mw2.prj archive and their file extensions, as MW2SHELL's mw2prj.c
// has them. In the original this object's data (the tables, their strings, then FirstResource's
// literals) follows the rest of resource.c's.
// GLOBAL: MW2 0x100a8674
char* g_resourceTypeTags[26] = {"SNDS", "CEL",  "XYC",  "SHP",  "FONT", "MENU", "DISP", "XMID", "PAL",
								"TABL", "POLY", "TEXT", "ANIM", "MGEO", "HUD",  "CPIT", "VPT",  "MPIT",
								"BWD",  "VER",  "AIT",  "MEK",  "LUMA", "MUS",  "GIF",  "NTXT"};

// GLOBAL: MW2 0x100a86dc
char* g_resourceTypeExtensions[25] = {".sfl", ".xel", ".xyc", ".shp", ".fnt", ".dll", ".dll", ".xmi", ".col",
									  ".tbl", ".wtb", ".xxt", ".3di", ".mgi", ".hdi", ".cpi", ".vpi", ".pit",
									  ".bwd", ".ait", ".mek", ".lum", ".mus", ".gif", ".txt"};

// GLOBAL: MW2 0x100a8740
MechS32 g_mw2PrjHandle = -1;

// GLOBAL: MW2 0x100a8744
char* g_mw2PrjPath = NULL;

// FUNCTION: MW2 0x10050780
MechS32 FirstResource(void)
{
	MechS32 result;

	result = TRUE;
	SetPrjAllocator(Mw2PrjAlloc, Mw2PrjFree);
	InitializeResourceCache();
	if (!g_mw2PrjPath) {
		g_mw2PrjPath = strdup(BuildGamePath("mw2.prj"));
	}

	g_mw2PrjHandle = OpenPrjFile(g_mw2PrjPath, 0);
	if (g_mw2PrjHandle != -1) {
		LoadPrjIndexes(g_mw2PrjHandle);
	}
	else {
		Error(3, "\nCan't find file \"%s\"", g_mw2PrjPath, 0);
		result = FALSE;
	}

	return result;
}

// FUNCTION: MW2 0x1005082f
void ShutdownMw2Prj(void)
{
	ClosePrjFile(g_mw2PrjHandle);
}

// FUNCTION: MW2 0x10050848
void CachePreloads(void)
{
	LoadWeaponSounds();
	PreloadCockpitSounds();
	LoadAIScripts();
}

// FUNCTION: MW2 0x10050862
MechS32 PreloadResource(MechS32 p_id, const char* p_type)
{
	MechS32 result;
	void* data;

	data = LoadCachedResource(g_mw2PrjHandle, p_id, p_type, 0);
	if (data) {
		result = TRUE;
		UnlockCachedResource(p_id, p_type);
	}
	else {
		result = FALSE;
	}

	return result;
}

// FUNCTION: MW2 0x100508c0
void* Mw2PrjAlloc(MechU32 p_size)
{
	return MemAlloc(p_size);
}

// FUNCTION: MW2 0x100508dc
void Mw2PrjFree(void* p_block)
{
	MechHeapFree(g_primaryHeap, p_block);
}
