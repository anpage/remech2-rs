/* The mission's sound files: a table of the names in the mission's project file, hashed by
   HashName, and the directory the files are read from. */
#include "missionaudio.h"

#include "decomp.h"
#include "error.h"
#include "gamecd.h"
#include "namehash.h"
#include "readfile.h"
#include "simmain.h"
#include "types.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

// A project file entry: a sound file name, chained in its hash bucket.
// SIZE 0x14
typedef struct ProjectFileEntry {
	MechChar m_name[0x10];           // 0x00
	struct ProjectFileEntry* m_next; // 0x10
} ProjectFileEntry;

DECOMP_SIZE_ASSERT(ProjectFileEntry, 0x14)

// GLOBAL: MW2 0x100a14f4
MechS32 g_soundFileCount = 0;

// GLOBAL: MW2 0x100bcdb8
static ProjectFileEntry g_soundFileEntries[200];

// GLOBAL: MW2 0x100bdd58
static ProjectFileEntry* g_soundFileTable[0x65];

// The directory of the mission's sound files.
// GLOBAL: MW2 0x100bdef0
MechChar g_soundFileDir[0x100];

// Returns the entry named p_name, or adds it if p_add; NULL when it is missing.
// Stack-slot permutation of entry, added and slot.
// FUNCTION: MW2 0x10007140
ProjectFileEntry* FindSoundFile(MechChar* p_name, MechS32 p_add)
{
	ProjectFileEntry* entry;
	ProjectFileEntry* added;
	MechU32 slot;

	slot = HashName(p_name) % 0x65;
	for (entry = g_soundFileTable[slot]; entry; entry = entry->m_next) {
		if (!_strcmpi(entry->m_name, p_name)) {
			return entry;
		}
	}

	if (!p_add) {
		return NULL;
	}

	entry = g_soundFileTable[slot];
	if (g_soundFileCount >= 200) {
		Error(0x23, "Too many project file entries", 0);
	}

	added = &g_soundFileEntries[g_soundFileCount++];
	strcpy(added->m_name, p_name);
	added->m_next = g_soundFileTable[slot];
	g_soundFileTable[slot] = added;
	return added;
}

// Lists the mission's sound files (keating\*.sfl, on the game CD when they aren't installed) in
// the project file table and keeps their directory in g_soundFileDir.
// FUNCTION: MW2 0x10007252
void CollectMissionAudio(void)
{
	MechChar drive;
	HANDLE find;
	WIN32_FIND_DATA data;

	drive = '\0';
	sprintf(g_soundFileDir, "%s\\*.sfl", "keating");
	find = FindFirstFile(g_soundFileDir, &data);
	if (find == INVALID_HANDLE_VALUE) {
		drive = FindGameCdDrive();
		if (!drive) {
			return;
		}

		sprintf(g_soundFileDir, "%c:\\%s\\*.sfl", drive, "keating");
		find = FindFirstFile(g_soundFileDir, &data);
		if (find == INVALID_HANDLE_VALUE) {
			return;
		}
	}

	for (;;) {
		if (!data.cAlternateFileName[0]) {
			FindSoundFile(data.cFileName, 1);
		}
		else {
			FindSoundFile(data.cAlternateFileName, 1);
		}

		if (!FindNextFile(find, &data)) {
			break;
		}
	}

	FindClose(find);
	if (drive) {
		sprintf(g_soundFileDir, "%c:\\%s", drive, "keating");
	}
	else {
		sprintf(g_soundFileDir, "%s", "keating");
	}
}

// Reads the sound file p_name (".sfl" appended) from the mission's directory, if the project
// file lists it. Returns the data, or NULL.
// Stack-slot permutation of path and name.
// FUNCTION: MW2 0x100073bb
void* ReadSoundFile(MechChar* p_name)
{
	MechChar path[0x100];
	MechChar name[0x100];

	if (!g_soundFileDir) {
		return NULL;
	}

	strcpy(name, p_name);
	strcat(name, ".sfl");
	if (!FindSoundFile(name, FALSE)) {
		return NULL;
	}

	strcpy(path, g_soundFileDir);
	strcat(path, "\\");
	strcat(path, name);
	return MechReadFile(g_primaryHeap, path);
}
