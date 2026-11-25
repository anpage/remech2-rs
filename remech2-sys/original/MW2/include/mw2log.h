#ifndef MW2LOG_H
#define MW2LOG_H

#include "types.h"

#include <stdio.h>

// The functions and globals of mw2log.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t OpenMw2Log(void);
	int32_t CloseMw2Log(void);
	int32_t WriteToMw2Log(char* p_text);

#ifdef __cplusplus
}
#endif

#endif // MW2LOG_H
