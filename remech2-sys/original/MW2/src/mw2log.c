#include "mw2log.h"

#include "files.h"
#include "log.h"
#include "types.h"

#include <stdio.h>
#include <time.h>

int32_t OpenMw2Log(void)
{
	time_t now;
	char line[80];

	now = time(NULL);
	sprintf(line, "MW2.EXE %s  %s \n", "$Date: 1995/06/25 15:47:38 $", ctime(&now));
	WriteToMw2Log(line);

	return TRUE;
}

int32_t CloseMw2Log(void)
{
	char line[80];
	time_t now;

	now = time(NULL);
	sprintf(line, "\nend of MW2  %s\n", ctime(&now));
	WriteToMw2Log(line);

	return TRUE;
}

int32_t WriteToMw2Log(char* p_text)
{
	MechLogTracef("mw2.log: %s", p_text);
	return TRUE;
}
