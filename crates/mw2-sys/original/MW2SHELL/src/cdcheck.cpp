#include "cdcheck.h"

#include "decomp.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// GLOBAL: MW2SHELL 0x10066ddc
MechChar g_cdDriveLetter = '\0';

// The game CD's drive among the CD-ROM drives, from 1; 0 if not found.
// GLOBAL: MW2SHELL 0x10066de0
MechS32 g_cdDriveNumber = 0;

// FUNCTION: MW2SHELL 0x10030a20
char CdCheck()
{
	WIN32_FIND_DATA findData;
	MechChar path[20];
	LPSTR driveStrings;
	HANDLE hFindFile;
	LPCSTR drive;

	if (g_cdDriveLetter != '\0') {
		return g_cdDriveLetter;
	}

	driveStrings = (LPSTR) calloc(0x69, 1);
	GetLogicalDriveStrings(0x69, driveStrings);
	sprintf(path, " :\\OLD_HERC.DRV");

	drive = driveStrings;
	g_cdDriveNumber = 0;
	while (*drive != '\0') {
		if (GetDriveType(drive) == DRIVE_CDROM) {
			g_cdDriveNumber++;
			path[0] = *drive;
			hFindFile = FindFirstFile(path, &findData);
			if (hFindFile != INVALID_HANDLE_VALUE) {
				FindClose(hFindFile);
				break;
			}
		}
		drive += 4;
	}

	if (*drive != '\0') {
		g_cdDriveLetter = *drive;
	}
	else {
		g_cdDriveNumber = 0;
	}

	free(driveStrings);
	return g_cdDriveLetter;
}

// Unused.
// FUNCTION: MW2SHELL 0x10030b5b
MechS32 GetCdDriveNumber()
{
	if (g_cdDriveNumber == 0) {
		CdCheck();
	}

	return g_cdDriveNumber;
}
