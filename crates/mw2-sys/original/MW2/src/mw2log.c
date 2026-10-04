#include "mw2log.h"

#include "decomp.h"
#include "types.h"

#include <stdio.h>
#include <time.h>
#include <windows.h>

// Set by the command line; SimMain opens mw2.log when it is.
// GLOBAL: MW2 0x100ae6d4
MechS32 g_logFileEnabled = 0;

// GLOBAL: MW2 0x100ae6d8
FILE* g_mw2Log = NULL;

// Opens mw2.log and writes the build date and the time to it. Returns whether it could.
// FUNCTION: MW2 0x100717e0
MechS32 OpenMw2Log(void)
{
	FILE* file = NULL;
	time_t now;
	MechChar line[80];

	file = fopen("mw2.log", "wt");
	if (file) {
		g_mw2Log = file;
		now = time(NULL);
		sprintf(line, "MW2.EXE %s  %s \n", "$Date: 1995/06/25 15:47:38 $", ctime(&now));
		WriteToMw2Log(line);
		return TRUE;
	}

	return FALSE;
}

// Writes the time to mw2.log and closes it. Returns whether it was open.
// Stack-slot permutation: line and now.
// FUNCTION: MW2 0x10071869
MechS32 CloseMw2Log(void)
{
	MechChar line[80];
	time_t now;

	if (g_mw2Log) {
		now = time(NULL);
		sprintf(line, "\nend of MW2  %s\n", ctime(&now));
		WriteToMw2Log(line);
		fclose(g_mw2Log);
		return TRUE;
	}

	return FALSE;
}

// Writes a line to mw2.log. Returns whether logging is on and the log is open.
// FUNCTION: MW2 0x100718da
MechS32 WriteToMw2Log(MechChar* p_text)
{
	if (g_logFileEnabled && g_mw2Log) {
		fprintf(g_mw2Log, "%s", p_text);
		return 1;
	}
	else {
		return 0;
	}
}
