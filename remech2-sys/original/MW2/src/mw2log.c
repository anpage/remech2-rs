#include "mw2log.h"

#include "decomp.h"
#include "files.h"
#include "log.h"
#include "types.h"

#include <stdio.h>
#include <time.h>

// FUNCTION: MW2 0x100717e0
MechS32 OpenMw2Log(void)
{
	time_t now;
	MechChar line[80];

	now = time(NULL);
	sprintf(line, "MW2.EXE %s  %s \n", "$Date: 1995/06/25 15:47:38 $", ctime(&now));
	WriteToMw2Log(line);

	return TRUE;
}

// FUNCTION: MW2 0x10071869
MechS32 CloseMw2Log(void)
{
	MechChar line[80];
	time_t now;

	now = time(NULL);
	sprintf(line, "\nend of MW2  %s\n", ctime(&now));
	WriteToMw2Log(line);

	return TRUE;
}

// FUNCTION: MW2 0x100718da
MechS32 WriteToMw2Log(MechChar* p_text)
{
	MechLogTracef("mw2.log: %s", p_text);
	return TRUE;
}
