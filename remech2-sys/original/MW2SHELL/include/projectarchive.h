#ifndef PROJECTARCHIVE_H
#define PROJECTARCHIVE_H

#include "starmech.h"
#include "types.h"

class ProjectArchive {
public:
	ProjectArchive(const char* p_name);
	~ProjectArchive();

	int32_t FindResourceId(char* p_name, int32_t p_type);
	void* GetResource(int32_t p_id, char* p_tag);
	void* GetResourceByName(char* p_name, int32_t p_type, char* p_tag);
	void ReleaseResource(int32_t p_id, char* p_tag);
	void ReleaseResourceByName(char* p_name, int32_t p_type, char* p_tag);
	int32_t* FindNextBwdNode(int32_t* p_list, int32_t p_index, int32_t* p_previous);
	uint8_t LoadBwd(char* p_name);
	int32_t* FindBwdNode(int32_t p_type);

private:
	int32_t m_handle;    // 0x00 — from OpenArchive
	uint32_t m_bwdTable; // 0x04 — the TABL resource that names the .bwd files
	int32_t* m_bwd;      // 0x08 — the .bwd LoadBwd loaded
	int32_t m_bwdSize;   // 0x0c
};

// The functions and globals of projectarchive.cpp that other units use.
extern char g_bwdTags[72][4];
extern int32_t g_enemyStarDifficulty;
extern int32_t g_bwdRegistrySize;
extern int32_t g_bwdLargestTemplate;
extern int16_t g_bwdDifficultySettings[9][5];
extern char* g_clanBitmapNames[6];
extern int32_t g_bwdTemplateRegistry[0x200];

void PrjWriteStarTemplates(int32_t p_count, StarMech* p_mechs, int32_t p_enemyCount, StarMech* p_enemies);
void PrjBuildPlayerStarTemplates(int32_t p_clan, int32_t p_rival);

// Implemented on the Rust side (src/shell/handoff.rs). Keeps a .bwd file for the next mission
extern "C" void MechKeepBwd(const char* p_name, const void* p_data, uint32_t p_size);

#endif // PROJECTARCHIVE_H
