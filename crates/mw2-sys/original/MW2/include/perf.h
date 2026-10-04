#ifndef PERF_H
#define PERF_H

#include "menu.h"
#include "menupage.h"
#include "types.h"

// The functions and globals of perf.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechChar g_combatVariablesItem[];
	extern MenuPage g_combatVariablesPage;

	void FirstPerfSetting(void);

#ifdef __cplusplus
}
#endif

#endif // PERF_H
