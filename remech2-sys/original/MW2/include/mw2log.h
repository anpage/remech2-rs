#ifndef MW2LOG_H
#define MW2LOG_H

#include "types.h"

#include <stdio.h>

// The functions and globals of mw2log.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 OpenMw2Log(void);
	MechS32 CloseMw2Log(void);
	MechS32 WriteToMw2Log(MechChar* p_text);

#ifdef __cplusplus
}
#endif

#endif // MW2LOG_H
