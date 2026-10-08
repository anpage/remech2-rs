#ifndef PROJECTARCHIVE_H
#define PROJECTARCHIVE_H

#include "decomp.h"
#include "starmech.h"
#include "types.h"

// SIZE 0x10
class ProjectArchive {
public:
	ProjectArchive(const char* p_name);
	~ProjectArchive();

	MechS32 FindResourceId(char* p_name, MechS32 p_type);
	void* GetResource(MechS32 p_id, char* p_tag);
	void* GetResourceByName(char* p_name, MechS32 p_type, char* p_tag);
	void ReleaseResource(MechS32 p_id, char* p_tag);
	void ReleaseResourceByName(char* p_name, MechS32 p_type, char* p_tag);
	MechS32* FindNextBwdNode(MechS32* p_list, MechS32 p_index, MechS32* p_previous);
	MechU8 LoadBwd(char* p_name);
	MechS32* FindBwdNode(MechS32 p_type);

private:
	MechS32 m_handle;      // 0x00 — from OpenArchive
	undefined4 m_bwdTable; // 0x04 — the TABL resource that names the .bwd files
	MechS32* m_bwd;        // 0x08 — the .bwd LoadBwd loaded
	MechS32 m_bwdSize;     // 0x0c
};

// The functions and globals of projectarchive.cpp that other units use.
extern char g_bwdTags[72][4];
extern MechS32 g_enemyStarDifficulty;
extern MechS32 g_bwdRegistrySize;
extern MechS32 g_bwdLargestTemplate;
extern MechS16 g_bwdDifficultySettings[9][5];
extern char* g_clanBitmapNames[6];
extern MechS32 g_bwdTemplateRegistry[0x200];

void PrjWriteStarTemplates(MechS32 p_count, StarMech* p_mechs, MechS32 p_enemyCount, StarMech* p_enemies);
void PrjBuildPlayerStarTemplates(MechS32 p_clan, MechS32 p_rival);

// Implemented on the Rust side (src/shell/handoff.rs). Keeps a .bwd file for the next mission
extern "C" void MechKeepBwd(const char* p_name, const void* p_data, MechU32 p_size);

#endif // PROJECTARCHIVE_H
