#ifndef DAMAGEPANEL_H
#define DAMAGEPANEL_H

#include "mech.h"
#include "targeting.h"
#include "types.h"

// The functions and globals of damagepanel.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern PANE g_outlinePartRects[16];
	extern MechS32 g_frameOutlineParts;

	void InitDamagePanel(void);
	void DrawDamageOutline(Mech* p_mech, PANE* p_target);
	void DrawArmorBars(Mech* p_mech, PANE* p_target);

#ifdef __cplusplus
}
#endif

#endif // DAMAGEPANEL_H
