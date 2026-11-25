#include "projectarchive.h"

#include "gamesettings.h"
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

// The simulator's BwdHeader.
struct BwdRootNode {
	int32_t m_type;        // 0x00
	int32_t m_size;        // 0x04
	int32_t m_maxNodeSize; // 0x08 — BwdAddRegistryTemplate keeps it at the largest node's size
};

struct BwdRevisionNode {
	int32_t m_type;     // 0x00
	int32_t m_size;     // 0x04
	char m_revision[4]; // 0x08
};

// The simulator's DtblRecord.
struct BwdTableNode {
	int32_t m_type;           // 0x00
	int32_t m_size;           // 0x04
	uint32_t m_poolCounts[5]; // 0x08 — the counts the static pools are sized for, all 0
};

// A bmid record: the simulator reads it as a ResourceRef.
struct BwdLinkNode {
	int32_t m_type;         // 0x00
	int32_t m_size;         // 0x04
	int16_t m_animation;    // 0x08 — the animation StartAnimation plays the bitmap before it in
	int16_t m_reserved0x0a; // 0x0a — the start of the unused ResourceRef::m_name
};

// One mech of a star, the simulator's IncludeRecord2 (gpspec); the registry takes its first 0x5a
// bytes.
struct BwdGpsNode {
	int32_t m_type;                      // 0x00
	int32_t m_size;                      // 0x04
	int16_t m_mekId;                     // 0x08 — the variant's MEK resource, -2 for a user variant
	int16_t m_id;                        // 0x0a — the chassis' gamepiece stream
	uint8_t m_team;                      // 0x0c — the star's level: 0 the player's, 1 to 5 the enemy's
	uint8_t m_leader;                    // 0x0d — 1: the first mech of the star
	uint8_t m_ai;                        // 0x0e — Player::m_aiMode: 0 for the player's own mech, else 2
	uint8_t m_reserved0x0f;              // 0x0f
	int16_t m_aiParams[8];               // 0x10 — Player::m_aiParams: g_bwdDifficultySettings' row, then 0
	int16_t m_events;                    // 0x20 — event types (WidenEventFlags)
	int16_t m_flags;                     // 0x22 — Player::m_flags
	char m_chassis[9];                   // 0x24
	char m_config[9];                    // 0x2d — the variant's file name
	char m_playerName[16];               // 0x36 — the pilot
	uint8_t m_reserved0x46[0x5c - 0x46]; // 0x46 — the rest of the player's name, and the short name
};

// A node whose size depends on the name it carries.
struct BwdNameNode {
	int32_t m_type;       // 0x00
	int32_t m_size;       // 0x04
	int16_t m_resourceId; // 0x08
	char m_name[1];       // 0x0a
};

// Heap callbacks registered with the archive unit (prjfile.c); the file unit's
// allocator calls them instead of the CRT heap.
void* PrjHeapAlloc(uint32_t p_size);
void PrjHeapFree(void* p_block);

void BwdAddRegistryTemplate(void* p_node, int32_t p_size);
void BwdInitRegistry();
void BwdWriteRegistry(char* p_fileName);
void PrjBuildMechVariantTemplate(
	char* p_variant,
	char* p_pilot,
	int32_t p_index,
	int32_t p_level,
	int32_t p_difficulty
);

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

int32_t g_bwdRegistrySize = 0;

int32_t g_bwdLargestTemplate = 0;

int16_t g_bwdDifficultySettings[9][5] = {
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
int32_t g_enemyStarDifficulty = 8;

// The clans' type 8 resources, by clan; the player's clan and rival go into instmap1.bwd.
char* g_clanBitmapNames[] = {"l1wolfcl", "l1jadefn", "l1gostbr", "l1smojag", "l1novact", "l1steelv"};

int32_t g_bwdTemplateRegistry[0x200];

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

void* PrjHeapAlloc(uint32_t p_size)
{
	return MechHeapAlloc(g_primaryHeap, p_size);
}

void PrjHeapFree(void* p_block)
{
	MechHeapFree(g_primaryHeap, p_block);
}

int32_t ProjectArchive::FindResourceId(char* p_name, int32_t p_type)
{
	if (m_handle < 0) {
		return -1;
	}

	return FindResourceIdByName(p_type, p_name);
}

void* ProjectArchive::GetResource(int32_t p_id, char* p_tag)
{
	if (p_id == -1 || p_id == -2) {
		return NULL;
	}

	return LoadCachedResource(m_handle, p_id, p_tag);
}

void* ProjectArchive::GetResourceByName(char* p_name, int32_t p_type, char* p_tag)
{
	return GetResource(FindResourceId(p_name, p_type), p_tag);
}

void ProjectArchive::ReleaseResource(int32_t p_id, char* p_tag)
{
	if (p_id == -1 || p_id == -2) {
		return;
	}

	FreeCachedResource(p_id, p_tag);
}

void ProjectArchive::ReleaseResourceByName(char* p_name, int32_t p_type, char* p_tag)
{
	ReleaseResource(FindResourceId(p_name, p_type), p_tag);
}

int32_t* ProjectArchive::FindNextBwdNode(int32_t* p_list, int32_t p_index, int32_t* p_previous)
{
	int32_t* node;
	int32_t* end;
	int32_t type;

	type = *(int32_t*) g_bwdTags[p_index];
	end = (int32_t*) ((char*) p_list + p_list[1]);
	if (p_previous != NULL) {
		node = (int32_t*) ((char*) p_previous + p_previous[1]);
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

uint8_t ProjectArchive::LoadBwd(char* p_name)
{
	int32_t id;

	m_bwdSize = 0;
	if (m_handle < 0) {
		return false;
	}

	id = FindResourceIdByName(m_bwdTable, p_name);
	switch (id) {
	case -1:
		return false;
	case -2:
		return false;
	default:
		m_bwd = (int32_t*) LoadCachedResource(m_handle, id, "BWD");
		m_bwdSize = m_bwd[1];
		if (m_bwd == NULL) {
			return false;
		}
	}

	return true;
}

int32_t* ProjectArchive::FindBwdNode(int32_t p_type)
{
	int32_t* node;
	int32_t offset;

	offset = 0xc;
	while (offset < m_bwdSize) {
		node = (int32_t*) ((char*) m_bwd + offset);
		if (*node == p_type) {
			return node;
		}

		offset += node[1];
	}

	return NULL;
}

ProjectArchive::~ProjectArchive()
{
	ShutdownResourceCache();
	if (m_handle < 0) {
		return;
	}

	CloseArchive(m_handle);
}

void BwdAddRegistryTemplate(void* p_node, int32_t p_size)
{
	char* destination;

	destination = (char*) g_bwdTemplateRegistry + g_bwdRegistrySize;
	if (g_bwdRegistrySize + p_size > (int32_t) sizeof(g_bwdTemplateRegistry)) {
		return;
	}

	memcpy(destination, p_node, p_size);
	p_size = (p_size + 3) & ~3;
	g_bwdRegistrySize += p_size;
	if (p_size > g_bwdLargestTemplate) {
		g_bwdTemplateRegistry[2] = g_bwdLargestTemplate = p_size;
	}
}

void BwdInitRegistry()
{
	BwdRootNode root;
	BwdRevisionNode revision;
	BwdTableNode table;

	memset(g_bwdTemplateRegistry, 0, sizeof(g_bwdTemplateRegistry));
	g_bwdRegistrySize = 0;
	g_bwdLargestTemplate = 0;

	root.m_type = *(int32_t*) g_bwdTags[c_tagBwd];
	root.m_size = 0;
	root.m_maxNodeSize = 0;
	BwdAddRegistryTemplate(&root, sizeof(root));

	revision.m_type = *(int32_t*) g_bwdTags[c_tagRev];
	revision.m_size = sizeof(revision);
	revision.m_revision[0] = '1';
	revision.m_revision[1] = '.';
	revision.m_revision[2] = '2';
	revision.m_revision[3] = '2';
	BwdAddRegistryTemplate(&revision, sizeof(revision));

	table.m_type = *(int32_t*) g_bwdTags[c_tagDtbl];
	table.m_size = sizeof(table);
	memset(table.m_poolCounts, 0, sizeof(table.m_poolCounts));
	BwdAddRegistryTemplate(&table, sizeof(table));
}

void BwdWriteRegistry(char* p_fileName)
{
	g_bwdTemplateRegistry[1] = g_bwdRegistrySize;
	MechKeepBwd(p_fileName, g_bwdTemplateRegistry, g_bwdRegistrySize);
}

// Adds the template of one mech of a star to the registry: its chassis, variant and pilot, and
// the settings of p_difficulty (1 to 8; 0 for a mech without a level).
void PrjBuildMechVariantTemplate(char* p_variant, char* p_pilot, int32_t p_index, int32_t p_level, int32_t p_difficulty)
{
	int32_t i;
	BwdGpsNode node;
	int16_t variant;

	for (i = 0; g_mechChassis[i].m_prefix; i++) {
		if (!strncasecmp(p_variant, g_mechChassis[i].m_prefix, 3)) {
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
	node.m_type = *(int32_t*) g_bwdTags[c_tagGps];
	node.m_size = sizeof(node);
	strncpy(node.m_config, p_variant, 8);
	node.m_config[8] = '\0';
	strncpy(node.m_chassis, g_mechChassis[i].m_chassis, 8);
	node.m_chassis[8] = '\0';
	strncpy(node.m_playerName, p_pilot, 15);
	node.m_playerName[15] = '\0';
	node.m_flags = 0x400;
	if (strncasecmp(p_variant + 5, "std", 3)) {
		variant = -2;
	}
	else {
		variant = FindResourceIdByName(6, node.m_config);
	}
	node.m_mekId = variant;
	node.m_id = FindResourceIdByName(14, node.m_chassis);
	node.m_team = p_level;
	if (p_index == 0) {
		node.m_leader = 1;
	}
	else {
		node.m_leader = 0;
	}
	if (p_index == 0 && p_level == 0) {
		node.m_ai = 0;
	}
	else {
		node.m_ai = 2;
	}
	node.m_aiParams[0] = g_bwdDifficultySettings[p_difficulty][0];
	node.m_aiParams[1] = g_bwdDifficultySettings[p_difficulty][1];
	node.m_aiParams[2] = g_bwdDifficultySettings[p_difficulty][2];
	node.m_aiParams[3] = g_bwdDifficultySettings[p_difficulty][3];
	node.m_aiParams[4] = g_bwdDifficultySettings[p_difficulty][4];
	node.m_aiParams[5] = 0;
	node.m_aiParams[6] = 0;
	node.m_aiParams[7] = 0;
	node.m_events = 6;
	BwdAddRegistryTemplate(&node, 0x5a);
}

void PrjWriteStarTemplates(int32_t p_count, StarMech* p_mechs, int32_t p_enemyCount, StarMech* p_enemies)
{
	int32_t i;
	int32_t level;
	int32_t difficulty;
	char fileName[0x10];
	DifficultySettings settings;

	BwdInitRegistry();
	for (i = 0; i < p_count; i++) {
		if (p_mechs[i].m_chassis >= 0) {
			PrjBuildMechVariantTemplate(p_mechs[i].m_variant, p_mechs[i].m_pilot, i, 0, 0);
		}
	}
	BwdWriteRegistry("userstar.bwd");

	difficulty = g_enemyStarDifficulty - 2;
	MechGetDifficultySettings(&settings);
	if (settings.m_enemySkill == 0) {
		difficulty++;
	}
	else if (settings.m_enemySkill == 2) {
		difficulty--;
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

void PrjBuildPlayerStarTemplates(int32_t p_clan, int32_t p_rival)
{
	int32_t size;
	BwdNameNode* node;
	char buffer[0x20];
	BwdLinkNode link;

	node = (BwdNameNode*) buffer;
	node->m_type = *(int32_t*) g_bwdTags[c_tagBmpj];
	link.m_type = *(int32_t*) g_bwdTags[c_tagBmid];
	link.m_size = sizeof(link);
	link.m_animation = 0;
	link.m_reserved0x0a = -1;
	BwdInitRegistry();

	node->m_resourceId = FindResourceIdByName(8, g_clanBitmapNames[p_clan]);
	strcpy(node->m_name, g_clanBitmapNames[p_clan]);
	size = strlen(g_clanBitmapNames[p_clan]) + 0x14;
	node->m_size = (size + 3) & ~3;
	BwdAddRegistryTemplate(node, (size + 3) & ~3);
	link.m_animation = 0x114;
	BwdAddRegistryTemplate(&link, sizeof(link));

	node->m_resourceId = FindResourceIdByName(8, g_clanBitmapNames[p_rival]);
	strcpy(node->m_name, g_clanBitmapNames[p_rival]);
	size = strlen(g_clanBitmapNames[p_rival]) + 0x14;
	node->m_size = (size + 3) & ~3;
	BwdAddRegistryTemplate(node, (size + 3) & ~3);
	link.m_animation = 0x115;
	BwdAddRegistryTemplate(&link, sizeof(link));

	node->m_resourceId = FindResourceIdByName(8, "jscamoia");
	strcpy(node->m_name, "jscamoia");
	size = strlen("jscamoia") + 0x14;
	node->m_size = (size + 3) & ~3;
	BwdAddRegistryTemplate(node, (size + 3) & ~3);
	link.m_animation = 0x101;
	BwdAddRegistryTemplate(&link, sizeof(link));

	BwdWriteRegistry("instmap1.bwd");
}
