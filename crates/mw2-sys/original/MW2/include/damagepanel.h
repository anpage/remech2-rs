#ifndef DAMAGEPANEL_H
#define DAMAGEPANEL_H

#include "mech.h"
#include "point.h"
#include "targeting.h"
#include "types.h"

// The functions and globals of damagepanel.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern PANE g_outlinePartRects[16];
	extern MechS32 g_frameOutlineParts;
	extern MechS32 g_outlinePartSections[16];
	extern Point g_htalLabelHPosition;
	extern Point g_htalLabelTPosition;
	extern Point g_htalLabelAPosition;
	extern Point g_htalLabelLPosition;
	extern MechS32 g_maxSectionArmor;
	extern PANE g_outlineRect;
	extern Point g_outlinePartOffsets[16];
	extern MechChar g_htalLabelH[4];
	extern MechChar g_htalLabelT[4];
	extern MechChar g_htalLabelA[4];
	extern MechChar g_htalLabelL[4];
	extern Point g_armorBarPositions[8];
	extern Point g_fullSectionArmor[8];
	extern MechU8 g_outlineRemap[0x100];
	extern Point g_armorBarSize;

	void InitDamagePanel(void);
	void DrawDamageOutline(Mech* p_mech, PANE* p_target);
	void DrawArmorBars(Mech* p_mech, PANE* p_target);

#ifdef __cplusplus
}
#endif

#endif // DAMAGEPANEL_H
