#include "damagepanel.h"

#include "bargauges.h"
#include "cockpit.h"
#include "cockpitpanel.h"
#include "config.h"
#include "decomp.h"
#include "environment.h"
#include "loadres.h"
#include "mech.h"
#include "mechdamage.h"
#include "mechsection.h"
#include "menu.h"
#include "muldiv.h"
#include "mw2prj.h"
#include "players.h"
#include "point.h"
#include "render.h"
#include "screenscale.h"
#include "setres.h"
#include "simmain.h"
#include "targeting.h"
#include "types.h"
#include "vfxa.h"

// The damage panel (panel 2): the mech's outline, its sections shaded by damage, and bars of each
// section's front and rear armor.

// The section each of the outline's sixteen parts shows, plus one (0: none).
// GLOBAL: MW2 0x100a5c78
MechS32 g_outlinePartSections[16] = {1, 3, 3, 2, 2, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 0};

// The places of the armor bars' four labels, in 16.16 fractions of the panel until InitDamagePanel
// scales them.

// GLOBAL: MW2 0x100a5cb8
Point g_htalLabelHPosition = {0x1e1e, 0x199a};

// GLOBAL: MW2 0x100a5cc0
Point g_htalLabelTPosition = {0x5050, 0x199a};

// GLOBAL: MW2 0x100a5cc8
Point g_htalLabelAPosition = {0x9697, 0x199a};

// GLOBAL: MW2 0x100a5cd0
Point g_htalLabelLPosition = {0xd2d3, 0x199a};

// The largest armor value of the local mech's sections.
// GLOBAL: MW2 0x100a5cd8
MechS32 g_maxSectionArmor = 0;

// The outline's rectangle, centered in the panel.
// GLOBAL: MW2 0x100a5ce0
PANE g_outlineRect = {NULL, 0, 0, 0, 0};

// The outline's sixteen parts: rectangles in the outline shape's pixels (LoadHudFile reads them)
// until InitDamagePanel places them on the screen.
// GLOBAL: MW2 0x100a5cf8
PANE g_outlinePartRects[16] = {0};

// Where each part's shape is drawn from, relative to its rectangle.
// GLOBAL: MW2 0x100a5e38
Point g_outlinePartOffsets[16] = {0};

// Frames the outline's parts (OutlinePane) when set.
// GLOBAL: MW2 0x100a5eb8
MechS32 g_frameOutlineParts = 0;

// The armor bars' labels.

// GLOBAL: MW2 0x100a5ebc
MechChar g_htalLabelH[4] = "H";

// GLOBAL: MW2 0x100a5ec0
MechChar g_htalLabelT[4] = "T";

// GLOBAL: MW2 0x100a5ec4
MechChar g_htalLabelA[4] = "A";

// GLOBAL: MW2 0x100a5ec8
MechChar g_htalLabelL[4] = "L";

// The armor bars' places, one per section (m_x the bar's left, m_y its bottom).
// GLOBAL: MW2 0x100be418
Point g_armorBarPositions[8];

// Each section's full front and rear armor.
// GLOBAL: MW2 0x100be458
Point g_fullSectionArmor[8];

// The remap table that colors an outline part (entry 6) by its damage.
// GLOBAL: MW2 0x100be498
MechU8 g_outlineRemap[0x100];

// The armor bars' width and full height.
// GLOBAL: MW2 0x100be598
Point g_armorBarSize;

// Sets up the damage panel: the identity remap table, the outline's rectangle and parts, the
// labels and armor bars' places, and the local mech's full armor.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10040020
void InitDamagePanel(void)
{
	CockpitPanel* panel;
	MechS32 color;
	MechS32 height2;
	MechS32 width2;
	MechS32 height1;
	MechS32 width1;
	void* shape;
	Mech* mech;
	MechS32 i;
	Point max;
	Point min;
	MechSection* section;

	panel = g_cockpitPanels[c_panelMechView];
	i = 0x100;
	while (i--) {
		g_outlineRemap[i] = i;
	}

	shape = LoadCachedResource(g_mw2PrjHandle, g_hudLayoutValues[0], g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		height1 = VFX_shape_resolution(shape, 0);
		UnlockCachedResource(g_hudLayoutValues[0], g_resourceTypeTags[c_resTagShp]);
		width1 = height1 >> 16;
		height1 &= 0xffff;
		shape = LoadCachedResource(
			g_mw2PrjHandle,
			g_hudLayoutValues[0] + g_artResolution,
			g_resourceTypeTags[c_resTagShp],
			0
		);
	}

	if (shape) {
		height2 = VFX_shape_resolution(shape, 0);
		UnlockCachedResource(g_hudLayoutValues[0] + g_artResolution, g_resourceTypeTags[c_resTagShp]);
		width2 = height2 >> 16;
		height2 &= 0xffff;
		g_outlineRect.m_window = &g_mainPixelBuffer;
		g_outlineRect.m_x0 = (panel->m_width - width2) / 2 + panel->m_x;
		g_outlineRect.m_y0 = (panel->m_height - height2) / 2 + panel->m_y;
		g_outlineRect.m_x1 = g_outlineRect.m_x0 + width2 - 1;
		g_outlineRect.m_y1 = g_outlineRect.m_y0 + height2 - 1;
		i = 16;
		while (i--) {
			g_outlinePartRects[i].m_window = &g_mainPixelBuffer;
			min.m_x = (MechDouble) g_outlinePartRects[i].m_x0 / width1 * 65536.0 + 0.5;
			min.m_y = (MechDouble) g_outlinePartRects[i].m_y0 / height1 * 65536.0 + 0.5;
			max.m_x = (MechDouble) g_outlinePartRects[i].m_x1 / width1 * 65536.0 + 0.5;
			max.m_y = (MechDouble) g_outlinePartRects[i].m_y1 / height1 * 65536.0 + 0.5;
			max.m_x += min.m_x - 1;
			max.m_y += min.m_y - 1;
			ScalePointToFrame(&g_outlineRect, &min, &min);
			ScalePointToFrame(&g_outlineRect, &max, &max);
			g_outlinePartRects[i].m_x0 = g_outlineRect.m_x0 + min.m_x;
			g_outlinePartRects[i].m_y0 = min.m_y + g_outlineRect.m_y0;
			g_outlinePartRects[i].m_x1 = g_outlineRect.m_x0 + max.m_x;
			g_outlinePartRects[i].m_y1 = max.m_y + g_outlineRect.m_y0;
			g_outlinePartOffsets[i].m_x = -min.m_x;
			g_outlinePartOffsets[i].m_y = -min.m_y;
		}
	}

	g_armorBarPositions[0].m_x = 0x2323;
	g_armorBarPositions[1].m_x = 0x7373;
	g_armorBarPositions[2].m_x = 0x5a5a;
	g_armorBarPositions[3].m_x = 0x4141;
	g_armorBarPositions[4].m_x = 0xaaab;
	g_armorBarPositions[5].m_x = 0x9192;
	g_armorBarPositions[6].m_x = 0xe1e2;
	g_armorBarPositions[7].m_x = 0xc8c9;
	ScalePointToFrame(panel->m_target, &g_htalLabelHPosition, &g_htalLabelHPosition);
	ScalePointToFrame(panel->m_target, &g_htalLabelTPosition, &g_htalLabelTPosition);
	ScalePointToFrame(panel->m_target, &g_htalLabelAPosition, &g_htalLabelAPosition);
	ScalePointToFrame(panel->m_target, &g_htalLabelLPosition, &g_htalLabelLPosition);
	color = 0x5555;
	i = 8;
	while (i--) {
		g_armorBarPositions[i].m_y = color;
		ScalePointToFrame(panel->m_target, &g_armorBarPositions[i], &g_armorBarPositions[i]);
	}

	mech = g_players[g_localPlayerId]->m_mech;
	g_armorBarSize.m_x = 0x1414;
	g_armorBarSize.m_y = 0x8000;
	ScalePointToFrame(panel->m_target, &g_armorBarSize, &g_armorBarSize);
	for (i = 0; i < 8; i++) {
		section = &mech->m_sections[i];
		g_fullSectionArmor[i].m_x = section->m_armor[0];
		g_fullSectionArmor[i].m_y = section->m_armor[1];
		if (g_fullSectionArmor[i].m_x > g_maxSectionArmor) {
			g_maxSectionArmor = g_fullSectionArmor[i].m_x;
		}

		if (g_fullSectionArmor[i].m_x > g_maxSectionArmor) {
			g_maxSectionArmor = g_fullSectionArmor[i].m_x;
		}
	}
}

// Draws the damage panel's outline, each part shaded by its section's damage: yellow, then red
// as its armor goes, black once the section is destroyed.
// The original loads m_sections before scaling index, and the locals are a stack-slot permutation.
// FUNCTION: MW2 0x10040511
void DrawDamageOutline(Mech* p_mech, PANE* p_target)
{
	MechS32 rear;
	MechS32 color;
	MechS32 front;
	MechS32 remap;
	void* shape;
	MechS32 scale;
	MechS32 level;
	MechS32 index;
	MechS32 i;
	MechSection* section;

	rear = 0;
	front = 0;
	shape =
		LoadCachedResource(g_mw2PrjHandle, g_hudLayoutValues[0] + g_artResolution, g_resourceTypeTags[c_resTagShp], 0);
	if (!shape) {
		return;
	}

	VFX_shape_draw(&g_outlineRect, shape, 0, 0, 0);
	for (i = 0; i < 16; i++) {
		rear = front = 0;
		if (g_outlinePartRects[i].m_x1 - g_outlinePartRects[i].m_x0 + 1 <= 0) {
			continue;
		}

		if (g_frameOutlineParts) {
			OutlinePane(&g_outlinePartRects[i], 0xe);
		}

		color = 6;
		index = g_outlinePartSections[i] - 1;
		section = &p_mech->m_sections[index];
		scale = (section->m_flags & 0xf0U) >> 4;
		if (scale) {
			front = 15 - (section->m_internal + section->m_armor[1] / g_localArmorPerLevel) * 3 / (scale << 16);
		}

		if (front < 1) {
			front = 0;
		}
		else if (front > 15) {
			front = 15;
		}

		scale = section->m_flags & 0xf;
		if (scale) {
			rear = 15 - (section->m_internal + section->m_armor[0] / g_localArmorPerLevel) * 3 / (scale << 16);
		}

		if (rear < 1) {
			rear = 0;
		}
		else if (rear > 15) {
			rear = 15;
		}

		level = front > rear ? front : rear;
		if (section->m_flags & 0x2000) {
			color = 0;
		}
		else if (level > 11) {
			color = 0xb;
		}
		else if (level > 0) {
			color = 3;
		}

		if (color != 6) {
			remap = color;
			g_outlineRemap[6] = remap;
			VFX_shape_lookaside(g_outlineRemap);
			VFX_shape_translate_draw(
				&g_outlinePartRects[i],
				shape,
				0,
				g_outlinePartOffsets[i].m_x,
				g_outlinePartOffsets[i].m_y
			);
		}
	}

	UnlockCachedResource(g_hudLayoutValues[0] + g_artResolution, g_resourceTypeTags[c_resTagShp]);
}

// Draws the damage panel's armor bars: each section's front armor, and the rear armor of the
// torso sections (1 to 3) beside it, full height for the section's full armor.
// The two full > armor tests take their operands in the other order, and the locals are a
// stack-slot permutation.
// FUNCTION: MW2 0x100407b6
void DrawArmorBars(Mech* p_mech, PANE* p_target)
{
	MechS32 full;
	MechS32 i;
	MechS32 color;
	void* font;
	MechS32 armor;
	MechS32 width;
	MechSection* section;

	g_textColors[0xe] = 6;
	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (font) {
		VFX_string_draw(p_target, g_htalLabelHPosition.m_x, g_htalLabelHPosition.m_y, font, g_htalLabelH, g_textColors);
		VFX_string_draw(p_target, g_htalLabelTPosition.m_x, g_htalLabelTPosition.m_y, font, g_htalLabelT, g_textColors);
		VFX_string_draw(p_target, g_htalLabelAPosition.m_x, g_htalLabelAPosition.m_y, font, g_htalLabelA, g_textColors);
		VFX_string_draw(p_target, g_htalLabelLPosition.m_x, g_htalLabelTPosition.m_y, font, g_htalLabelL, g_textColors);
		g_textColors[0xe] = 0xe;
		UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
	}

	for (i = 0; i < 8; i++) {
		section = &p_mech->m_sections[i];
		width = g_armorBarSize.m_x;
		if (i > 0 && i < 4) {
			width /= 2;
		}

		if (section->m_flags & 0x2000) {
			armor = 0;
			color = 0xf3;
		}
		else {
			armor = section->m_armor[0];
			if (g_fullSectionArmor[i].m_x >> 2 >= armor) {
				color = 0xb;
			}
			else {
				color = 3;
			}

			armor = MulDiv64(armor, g_armorBarSize.m_y, g_maxSectionArmor);
		}

		full = MulDiv64(g_fullSectionArmor[i].m_x, g_armorBarSize.m_y, g_maxSectionArmor);
		if (armor) {
			DrawVerticalBar(
				p_target,
				g_armorBarPositions[i].m_x,
				g_armorBarPositions[i].m_y + armor,
				width,
				armor,
				0xf
			);
		}

		if (full > armor) {
			DrawVerticalBar(
				p_target,
				g_armorBarPositions[i].m_x,
				g_armorBarPositions[i].m_y + full,
				width,
				full - armor,
				color
			);
		}

		if (i > 0 && i < 4) {
			if (section->m_flags & 0x2000) {
				armor = 0;
				color = 0xf3;
			}
			else {
				armor = section->m_armor[1];
				if (g_fullSectionArmor[i].m_y >> 2 >= armor) {
					color = 0xb;
				}
				else {
					color = 3;
				}

				armor = MulDiv64(armor, g_armorBarSize.m_y, g_maxSectionArmor);
			}

			full = MulDiv64(g_fullSectionArmor[i].m_y, g_armorBarSize.m_y, g_maxSectionArmor);
			if (armor) {
				DrawVerticalBar(
					p_target,
					g_armorBarPositions[i].m_x + width,
					g_armorBarPositions[i].m_y + armor,
					width,
					armor,
					0xf
				);
			}

			if (full > armor) {
				DrawVerticalBar(
					p_target,
					g_armorBarPositions[i].m_x + width,
					g_armorBarPositions[i].m_y + full,
					width,
					full - armor,
					color
				);
			}
		}
	}
}
