#include "mw2prj.h"

#include "decomp.h"
#include "prjfile.h"
#include "resourcecache.h"
#include "resourcefile.h"
#include "types.h"

#include <mbstring.h>
#include <stdlib.h>

void* Mw2PrjAlloc(undefined4 p_size);
void Mw2PrjFree(void* p_mem);

// Resource type tags in the mw2.prj archive and their file extensions.
// GLOBAL: MW2SHELL 0x1006a9f8
char* g_resourceTypeTags[26] = {"SNDS", "CEL",  "XYC",  "SHP",  "FONT", "MENU", "DISP", "XMID", "PAL",
								"TABL", "POLY", "TEXT", "ANIM", "MGEO", "HUD",  "CPIT", "VPT",  "MPIT",
								"BWD",  "VER",  "AIT",  "MEK",  "LUMA", "MUS",  "GIF",  "NTXT"};

// GLOBAL: MW2SHELL 0x1006aa60
char* g_resourceTypeExtensions[25] = {".sfl", ".xel", ".xyc", ".shp", ".fnt", ".dll", ".dll", ".xmi", ".col",
									  ".tbl", ".wtb", ".xxt", ".3di", ".mgi", ".hdi", ".cpi", ".vpi", ".pit",
									  ".bwd", ".ait", ".mek", ".lum", ".mus", ".gif", ".txt"};

// GLOBAL: MW2SHELL 0x1006aac4
MechS32 g_mw2PrjHandle = -1;

// GLOBAL: MW2SHELL 0x1006aac8
char* g_mw2PrjPath = NULL;

// FUNCTION: MW2SHELL 0x1003bfb0
MechS32 InitializeMw2Prj(void)
{
	MechS32 result;

	result = TRUE;
	SetArchiveAllocator(Mw2PrjAlloc, Mw2PrjFree);
	InitializeResourceCache();

	if (g_mw2PrjPath == NULL) {
		g_mw2PrjPath = (char*) _mbsdup((unsigned char*) MakeResourcePath("mw2.prj"));
	}

	g_mw2PrjHandle = OpenArchive(g_mw2PrjPath, 0);
	if (g_mw2PrjHandle != -1) {
		LoadArchiveEntries(g_mw2PrjHandle);
	}
	else {
		result = FALSE;
	}

	return result;
}

// FUNCTION: MW2SHELL 0x1003c048
void ShutdownMw2Prj(void)
{
	CloseArchive(g_mw2PrjHandle);
}

// FUNCTION: MW2SHELL 0x1003c061
void* Mw2PrjAlloc(undefined4 p_size)
{
	return AllocateMemory(p_size);
}

// FUNCTION: MW2SHELL 0x1003c07d
void Mw2PrjFree(void* p_mem)
{
	free(p_mem);
}
