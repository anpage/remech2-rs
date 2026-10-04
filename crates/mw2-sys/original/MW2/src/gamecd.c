#include "gamecd.h"

#include "simmain.h"
#include "types.h"

#include <stdio.h>
#include <windows.h>

// GLOBAL: MW2 0x100a00f8
MechChar g_gameCdDrive = 0;

// GLOBAL: MW2 0x100a00fc
MechS32 g_gameCdNumber = 0;

// Finds the CD-ROM drive holding the game disc (the one with \OLD_HERC.DRV) and caches its
// letter; g_gameCdNumber counts the CD-ROM drives up to it.
// FUNCTION: MW2 0x10002cb0
MechChar FindGameCdDrive(void)
{
	LPSTR drive;
	HANDLE find;
	LPSTR drives;
	MechChar path[20];
	WIN32_FIND_DATA findData;

	if (g_gameCdDrive) {
		return g_gameCdDrive;
	}

	drives = HeapAlloc(g_primaryHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, 0x69);
	GetLogicalDriveStrings(0x69, drives);
	sprintf(path, " :\\OLD_HERC.DRV");
	drive = drives;
	g_gameCdNumber = 0;
	while (*drive) {
		if (GetDriveType(drive) == DRIVE_CDROM) {
			g_gameCdNumber++;
			path[0] = *drive;
			find = FindFirstFile(path, &findData);
			if (find != INVALID_HANDLE_VALUE) {
				FindClose(find);
				break;
			}
		}

		drive += 4;
	}

	if (*drive) {
		g_gameCdDrive = *drive;
	}
	else {
		g_gameCdNumber = 0;
	}

	HeapFree(g_primaryHeap, HEAP_NO_SERIALIZE, drives);
	return g_gameCdDrive;
}

// FUNCTION: MW2 0x10002df5
MechS32 GetGameCdNumber(void)
{
	if (g_gameCdNumber == 0) {
		FindGameCdDrive();
	}

	return g_gameCdNumber;
}
