#include "projectarchive.h"

#include "decomp.h"
#include "difficultyconfig.h"
#include "files.h"
#include "mechbay.h"
#include "mechchassis.h"
#include "mw2prj.h"
#include "prjfile.h"
#include "resourcecache.h"
#include "resourcename.h"
#include "starmech.h"
#include "types.h"
#include "windowstate.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

DECOMP_SIZE_ASSERT(ProjectArchive, 0x10)
DECOMP_SIZE_ASSERT(MechChassis, 0x18)

// Every node of a .bwd file starts with a type tag from g_bwdTags and the node's
// size in bytes. The shell assembles .bwd files for the simulator in g_bwdTemplateRegistry.

enum BwdTag {
	c_tagBwd = 0,
	c_tagRev = 1,
	c_tagDtbl = 2,
	c_tagBmpj = 14,
	c_tagBmid = 15,
	c_tagGps = 52
};

// SIZE 0x0c
struct BwdRootNode {
	MechS32 m_type;       // 0x00
	MechS32 m_size;       // 0x04
	undefined4 m_unk0x08; // 0x08 — written as 0 for the simulator
};

// SIZE 0x0c
struct BwdRevisionNode {
	MechS32 m_type;     // 0x00
	MechS32 m_size;     // 0x04
	char m_revision[4]; // 0x08
};

// SIZE 0x1c
struct BwdTableNode {
	MechS32 m_type;          // 0x00
	MechS32 m_size;          // 0x04
	undefined4 m_unk0x08[5]; // 0x08 — written as zeros for the simulator
};

// SIZE 0x0c
struct BwdLinkNode {
	MechS32 m_type;    // 0x00
	MechS32 m_size;    // 0x04
	MechS16 m_unk0x08; // 0x08 — 0x114, 0x115 or 0x101 after each bitmap name node
	MechS16 m_unk0x0a; // 0x0a — written as -1 for the simulator
};

// SIZE 0x5c
// One mech of a star; the registry takes its first 0x5a bytes.
struct BwdGpsNode {
	MechS32 m_type;                   // 0x00
	MechS32 m_size;                   // 0x04
	MechS16 m_variantId;              // 0x08 — the variant's resource, -2 for a user variant
	MechS16 m_chassisId;              // 0x0a — the chassis' resource
	MechU8 m_level;                   // 0x0c
	MechU8 m_firstInStar;             // 0x0d
	MechU8 m_unk0x0e;                 // 0x0e — 0 for a star's leader at level 0, else 2; written for the simulator
	undefined m_unk0x0f;              // 0x0f — never written, the node is cleared first
	MechS16 m_difficultySettings[5];  // 0x10 — g_bwdDifficultySettings' row
	MechS16 m_unk0x1a;                // 0x1a — written as 0 for the simulator
	MechS16 m_unk0x1c;                // 0x1c — written as 0 for the simulator
	MechS16 m_unk0x1e;                // 0x1e — written as 0 for the simulator
	MechS16 m_unk0x20;                // 0x20 — written as 6 for the simulator
	MechS16 m_unk0x22;                // 0x22 — written as 0x400 for the simulator
	char m_chassis[9];                // 0x24
	char m_mech[9];                   // 0x2d
	char m_variant[16];               // 0x36
	undefined m_unk0x46[0x5c - 0x46]; // 0x46 — cleared, never written
};

// A node whose size depends on the name it carries.
struct BwdNameNode {
	MechS32 m_type;       // 0x00
	MechS32 m_size;       // 0x04
	MechS16 m_resourceId; // 0x08
	char m_name[1];       // 0x0a
};

// Heap callbacks registered with the archive unit (prjfile.c); the file unit's
// allocator calls them instead of the CRT heap.
void* PrjHeapAlloc(undefined4 p_size);
void PrjHeapFree(void* p_block);

void BwdAddRegistryTemplate(void* p_node, MechS32 p_size);
void BwdInitRegistry();
void BwdWriteRegistry(char* p_fileName);
void PrjBuildMechVariantTemplate(char* p_mech, char* p_variant, MechS32 p_index, MechS32 p_level, MechS32 p_difficulty);

// GLOBAL: MW2SHELL 0x100668c0
char g_bwdTags[][4] = {
	{'B', 'W', 'D', 0},   {'R', 'E', 'V', 0},   {'D', 'T', 'B', 'L'}, {'P', 'L', 'N', 'T'}, {'P', 'A', 'L', 'G'},
	{'T', 'E', 'R', 'R'}, {'C', 'L', 'I', 'M'}, {'L', 'I', 'T', 'E'}, {'V', 'W', 'S', 'P'}, {'V', 'W', 'S', 'T'},
	{'I', 'N', 'I', 'T'}, {'S', 'T', 'B', 'L'}, {'M', 'T', 'B', 'L'}, {'G', 'T', 'B', 'L'}, {'B', 'M', 'P', 'J'},
	{'B', 'M', 'I', 'D'}, {'B', 'S', 'E', 'C'}, {'B', 'M', 'E', 'N'}, {'F', 'P', 'R', 'J'}, {'H', 'R', 'Z', 'M'},
	{'G', 'N', 'D', 'M'}, {'S', 'K', 'Y', 'M'}, {'P', 'L', 'G', 'O'}, {'B', 'L', 'K', 'X'}, {'R', 'E', 'P', 'R'},
	{'E', 'N', 'D', 'R'}, {'B', 'L', 'K', 0},   {'E', 'L', 'S', 'B'}, {'E', 'N', 'D', 'B'}, {'O', 'B', 'J', 0},
	{'A', 'N', 'I', 'M'}, {'S', 'C', 'R', 'G'}, {'T', 'H', 'N', 'G'}, {'G', 'P', 0, 0},     {'C', 'P', 'T', 'F'},
	{'P', 'I', 'T', 'F'}, {'V', 'P', 'T', 'F'}, {'H', 'U', 'D', 'F'}, {'M', 'G', 'D', 'F'}, {'E', 'Y', 'E', 'O'},
	{'G', 'T', 0, 0},     {'O', 'B', 'J', 'L'}, {'B', 'T', 'H', 'G'}, {'X', 'P', 'L', 'O'}, {'N', 'A', 'V', 'P'},
	{'N', 'A', 'V', 'O'}, {'L', 'I', 'T', 'O'}, {'T', 'S', 'K', 0},   {'P', 'O', 'S', 0},   {'R', 'O', 'T', 0},
	{'I', 'N', 'C', 'L'}, {'G', 'R', 'P', 0},   {'G', 'P', 'S', 0},   {'M', 'O', 'F', 'F'}, {'M', 'O', 'N', 0},
	{'G', 'O', 'N', 0},   {'G', 'O', 'F', 'F'}, {'A', 'N', 'M', '2'}, {'S', 'T', 'A', 'R'}, {'V', 'I', 'E', 'W'},
	{'P', 'O', 'F', 'O'}, {'A', 'F', 'F', 'L'}, {'O', 'R', 'D', 'R'}, {'M', 'U', 'S', 'I'}, {'A', 'S', 'N', 'D'},
	{'L', 'T', 'B', 'L'}, {'P', 'T', 'B', 'L'}, {'F', 'T', 'B', 'L'}, {'H', 'T', 'X', 'T'}, {'P', 'D', 'S', 'C'},
	{'S', 'D', 'S', 'C'}, {'S', 'U', 'P', 'S'},
};

// GLOBAL: MW2SHELL 0x100669e0
MechS32 g_bwdRegistrySize = 0;

// GLOBAL: MW2SHELL 0x100669e4
MechS32 g_bwdLargestTemplate = 0;

// GLOBAL: MW2SHELL 0x100669e8
MechS16 g_bwdDifficultySettings[9][5] = {
	{0, 0, 0, 0, 0},
	{1, 850, 1, 850, 1},
	{2, 700, 1, 700, 2},
	{3, 650, 1, 650, 2},
	{3, 550, 1, 550, 3},
	{4, 450, 1, 450, 3},
	{4, 350, 1, 350, 4},
	{5, 300, 1, 300, 4},
	{5, 250, 1, 250, 5}
};

// The enemy star's difficulty, from the mission's BWD file (ShellApplyMissionUiInfo).
// PrjWriteStarTemplates starts the enemy templates at this less 2, adjusted by the enemy skill.
// GLOBAL: MW2SHELL 0x10066a44
MechS32 g_enemyStarDifficulty = 8;

// The clans' type 8 resources, by clan; the player's clan and rival go into instmap1.bwd.
// GLOBAL: MW2SHELL 0x10066a48
char* g_clanBitmapNames[] = {"l1wolfcl", "l1jadefn", "l1gostbr", "l1smojag", "l1novact", "l1steelv"};

// GLOBAL: MW2SHELL 0x1008f758
MechS32 g_bwdTemplateRegistry[0x200];

// FUNCTION: MW2SHELL 0x1002e280
ProjectArchive::ProjectArchive(const char* p_name)
{
	m_bwdTable = 0xe;
	m_bwd = NULL;
	SetArchiveAllocator(PrjHeapAlloc, PrjHeapFree);
	InitializeResourceCache();
	m_handle = OpenArchive((char*) p_name, '\0');
	if (m_handle < 0) {
		return;
	}

	g_mw2PrjHandle = m_handle;
	LoadArchiveEntries(g_mw2PrjHandle);
}

// FUNCTION: MW2SHELL 0x1002e302
void* PrjHeapAlloc(undefined4 p_size)
{
	return MechHeapAlloc(g_primaryHeap, p_size);
}

// FUNCTION: MW2SHELL 0x1002e324
void PrjHeapFree(void* p_block)
{
	MechHeapFree(g_primaryHeap, p_block);
}

// FUNCTION: MW2SHELL 0x1002e346
MechS32 ProjectArchive::FindResourceId(char* p_name, MechS32 p_type)
{
	if (m_handle < 0) {
		return -1;
	}

	return FindResourceIdByName(p_type, p_name);
}

// FUNCTION: MW2SHELL 0x1002e384
void* ProjectArchive::GetResource(MechS32 p_id, char* p_tag)
{
	if (p_id == -1 || p_id == -2) {
		return NULL;
	}

	return LoadCachedResource(m_handle, p_id, p_tag, 0);
}

// FUNCTION: MW2SHELL 0x1002e3cf
void* ProjectArchive::GetResourceByName(char* p_name, MechS32 p_type, char* p_tag)
{
	return GetResource(FindResourceId(p_name, p_type), p_tag);
}

// FUNCTION: MW2SHELL 0x1002e404
void ProjectArchive::ReleaseResource(MechS32 p_id, char* p_tag)
{
	if (p_id == -1 || p_id == -2) {
		return;
	}

	FreeCachedResource(p_id, p_tag);
}

// FUNCTION: MW2SHELL 0x1002e445
void ProjectArchive::ReleaseResourceByName(char* p_name, MechS32 p_type, char* p_tag)
{
	ReleaseResource(FindResourceId(p_name, p_type), p_tag);
}

// Stack-slot permutation: node and end.
// FUNCTION: MW2SHELL 0x1002e47a
MechS32* ProjectArchive::FindNextBwdNode(MechS32* p_list, MechS32 p_index, MechS32* p_previous)
{
	MechS32* node;
	MechS32* end;
	MechS32 type;

	type = *(MechS32*) g_bwdTags[p_index];
	end = (MechS32*) ((char*) p_list + p_list[1]);
	if (p_previous != NULL) {
		node = (MechS32*) ((char*) p_previous + p_previous[1]);
	}
	else {
		node = p_list + 3;
	}

	for (; node < end; node += node[1] / 4) {
		if (*node == type) {
			return node;
		}
	}

	return NULL;
}

// FUNCTION: MW2SHELL 0x1002e512
MechU8 ProjectArchive::LoadBwd(char* p_name)
{
	MechS32 id;

	m_bwdSize = 0;
	if (m_handle < 0) {
		return FALSE;
	}

	id = FindResourceIdByName(m_bwdTable, p_name);
	switch (id) {
	case -1:
		return FALSE;
	case -2:
		return FALSE;
	default:
		m_bwd = (MechS32*) LoadCachedResource(m_handle, id, "BWD", 0);
		m_bwdSize = m_bwd[1];
		if (m_bwd == NULL) {
			return FALSE;
		}
	}

	return TRUE;
}

// FUNCTION: MW2SHELL 0x1002e5d8
MechS32* ProjectArchive::FindBwdNode(MechS32 p_type)
{
	MechS32* node;
	MechS32 offset;

	offset = 0xc;
	while (offset < m_bwdSize) {
		node = (MechS32*) ((char*) m_bwd + offset);
		if (*node == p_type) {
			return node;
		}

		offset += node[1];
	}

	return NULL;
}

// FUNCTION: MW2SHELL 0x1002e638
ProjectArchive::~ProjectArchive()
{
	MechS32 result;

	ShutdownResourceCache();
	if (m_handle < 0) {
		return;
	}

	result = CloseArchive(m_handle);
	if (result < 0) {
	}
}

// Operand order: the original loads g_bwdLargestTemplate first for p_size > g_bwdLargestTemplate.
// FUNCTION: MW2SHELL 0x1002e67f
void BwdAddRegistryTemplate(void* p_node, MechS32 p_size)
{
	char* destination;

	destination = (char*) g_bwdTemplateRegistry + g_bwdRegistrySize;
	if (g_bwdRegistrySize + p_size > (MechS32) sizeof(g_bwdTemplateRegistry)) {
		return;
	}

	memcpy(destination, p_node, p_size);
	p_size = (p_size + 3) & ~3;
	g_bwdRegistrySize += p_size;
	if (p_size > g_bwdLargestTemplate) {
		g_bwdTemplateRegistry[2] = g_bwdLargestTemplate = p_size;
	}
}

// Stack-slot permutation: root, revision and table.
// FUNCTION: MW2SHELL 0x1002e703
void BwdInitRegistry()
{
	BwdRootNode root;
	BwdRevisionNode revision;
	BwdTableNode table;

	memset(g_bwdTemplateRegistry, 0, sizeof(g_bwdTemplateRegistry));
	g_bwdRegistrySize = 0;
	g_bwdLargestTemplate = 0;

	root.m_type = *(MechS32*) g_bwdTags[c_tagBwd];
	root.m_size = 0;
	root.m_unk0x08 = 0;
	BwdAddRegistryTemplate(&root, sizeof(root));

	revision.m_type = *(MechS32*) g_bwdTags[c_tagRev];
	revision.m_size = sizeof(revision);
	revision.m_revision[0] = '1';
	revision.m_revision[1] = '.';
	revision.m_revision[2] = '2';
	revision.m_revision[3] = '2';
	BwdAddRegistryTemplate(&revision, sizeof(revision));

	table.m_type = *(MechS32*) g_bwdTags[c_tagDtbl];
	table.m_size = sizeof(table);
	memset(table.m_unk0x08, 0, sizeof(table.m_unk0x08));
	BwdAddRegistryTemplate(&table, sizeof(table));
}

// FUNCTION: MW2SHELL 0x1002e7cb
void BwdWriteRegistry(char* p_fileName)
{
	g_bwdTemplateRegistry[1] = g_bwdRegistrySize;
	MechS32 file = MechOpen(p_fileName, c_mechOpenWrite);
	if (file == -1) {
		return;
	}

	MechWrite(file, g_bwdTemplateRegistry, g_bwdRegistrySize);
	MechClose(file);
}

// Adds the template of one mech of a star to the registry: its chassis and variant, and the
// settings of p_difficulty (1 to 8; 0 for a mech without a level).
// Not 100%: the stack slots of i and variant are permuted.
// FUNCTION: MW2SHELL 0x1002e830
void PrjBuildMechVariantTemplate(char* p_mech, char* p_variant, MechS32 p_index, MechS32 p_level, MechS32 p_difficulty)
{
	MechS32 i;
	BwdGpsNode node;
	MechS16 variant;

	for (i = 0; g_mechChassis[i].m_prefix; i++) {
		if (!strncasecmp(p_mech, g_mechChassis[i].m_prefix, 3)) {
			break;
		}
	}

	if (!g_mechChassis[i].m_prefix) {
		return;
	}

	if (p_difficulty < 1) {
		p_difficulty = 1;
	}
	if (p_difficulty > 8) {
		p_difficulty = 8;
	}
	if (!p_level) {
		p_difficulty = 0;
	}

	memset(&node, 0, 0x5a);
	node.m_type = *(MechS32*) g_bwdTags[c_tagGps];
	node.m_size = sizeof(node);
	strncpy(node.m_mech, p_mech, 8);
	node.m_mech[8] = '\0';
	strncpy(node.m_chassis, g_mechChassis[i].m_chassis, 8);
	node.m_chassis[8] = '\0';
	strncpy(node.m_variant, p_variant, 15);
	node.m_variant[15] = '\0';
	node.m_unk0x22 = 0x400;
	if (strncasecmp(p_mech + 5, "std", 3)) {
		variant = -2;
	}
	else {
		variant = FindResourceIdByName(6, node.m_mech);
	}
	node.m_variantId = variant;
	node.m_chassisId = FindResourceIdByName(14, node.m_chassis);
	node.m_level = p_level;
	if (p_index == 0) {
		node.m_firstInStar = 1;
	}
	else {
		node.m_firstInStar = 0;
	}
	if (p_index == 0 && p_level == 0) {
		node.m_unk0x0e = 0;
	}
	else {
		node.m_unk0x0e = 2;
	}
	node.m_difficultySettings[0] = g_bwdDifficultySettings[p_difficulty][0];
	node.m_difficultySettings[1] = g_bwdDifficultySettings[p_difficulty][1];
	node.m_difficultySettings[2] = g_bwdDifficultySettings[p_difficulty][2];
	node.m_difficultySettings[3] = g_bwdDifficultySettings[p_difficulty][3];
	node.m_difficultySettings[4] = g_bwdDifficultySettings[p_difficulty][4];
	node.m_unk0x1a = 0;
	node.m_unk0x1c = 0;
	node.m_unk0x1e = 0;
	node.m_unk0x20 = 6;
	BwdAddRegistryTemplate(&node, 0x5a);
}

// Operand order: the original loads i first for both i < count comparisons; the locals
// also take different [ebp-N] slots.
// FUNCTION: MW2SHELL 0x1002ea62
void PrjWriteStarTemplates(MechS32 p_count, StarMech* p_mechs, MechS32 p_enemyCount, StarMech* p_enemies)
{
	MechS32 i;
	MechS32 level;
	MechS32 difficulty;
	char fileName[0x10];
	DifficultyConfig settings;

	BwdInitRegistry();
	for (i = 0; i < p_count; i++) {
		if (p_mechs[i].m_chassis >= 0) {
			PrjBuildMechVariantTemplate(p_mechs[i].m_variant, p_mechs[i].m_pilot, i, 0, 0);
		}
	}
	BwdWriteRegistry("userstar.bwd");

	difficulty = g_enemyStarDifficulty - 2;
	MechS32 file = MechOpen("MW2DIF.CFG", c_mechOpenRead);
	if (file != -1) {
		MechRead(file, &settings, sizeof(settings));
		MechClose(file);
		if (settings.m_enemySkill == 0) {
			difficulty++;
		}
		else if (settings.m_enemySkill == 2) {
			difficulty--;
		}
	}

	for (level = 1; level <= 5; level++, difficulty--) {
		BwdInitRegistry();
		for (i = 0; i < p_enemyCount; i++) {
			if (p_enemies[i].m_chassis >= 0) {
				PrjBuildMechVariantTemplate(p_enemies[i].m_variant, p_enemies[i].m_pilot, i, level, difficulty);
			}
		}

		sprintf(fileName, "en%02dstar.bwd", level);
		BwdWriteRegistry(fileName);
	}
}

// Stack-slot permutation: size, node, buffer and link.
// FUNCTION: MW2SHELL 0x1002ec0e
void PrjBuildPlayerStarTemplates(MechS32 p_clan, MechS32 p_rival)
{
	MechS32 size;
	BwdNameNode* node;
	char buffer[0x20];
	BwdLinkNode link;

	node = (BwdNameNode*) buffer;
	node->m_type = *(MechS32*) g_bwdTags[c_tagBmpj];
	link.m_type = *(MechS32*) g_bwdTags[c_tagBmid];
	link.m_size = sizeof(link);
	link.m_unk0x08 = 0;
	link.m_unk0x0a = -1;
	BwdInitRegistry();

	node->m_resourceId = FindResourceIdByName(8, g_clanBitmapNames[p_clan]);
	strcpy(node->m_name, g_clanBitmapNames[p_clan]);
	size = strlen(g_clanBitmapNames[p_clan]) + 0x14;
	node->m_size = (size + 3) & ~3;
	BwdAddRegistryTemplate(node, (size + 3) & ~3);
	link.m_unk0x08 = 0x114;
	BwdAddRegistryTemplate(&link, sizeof(link));

	node->m_resourceId = FindResourceIdByName(8, g_clanBitmapNames[p_rival]);
	strcpy(node->m_name, g_clanBitmapNames[p_rival]);
	size = strlen(g_clanBitmapNames[p_rival]) + 0x14;
	node->m_size = (size + 3) & ~3;
	BwdAddRegistryTemplate(node, (size + 3) & ~3);
	link.m_unk0x08 = 0x115;
	BwdAddRegistryTemplate(&link, sizeof(link));

	node->m_resourceId = FindResourceIdByName(8, "jscamoia");
	strcpy(node->m_name, "jscamoia");
	size = strlen("jscamoia") + 0x14;
	node->m_size = (size + 3) & ~3;
	BwdAddRegistryTemplate(node, (size + 3) & ~3);
	link.m_unk0x08 = 0x101;
	BwdAddRegistryTemplate(&link, sizeof(link));

	BwdWriteRegistry("instmap1.bwd");
}
