#include "mechbay.h"

#include "audiosample.h"
#include "buttonmenu.h"
#include "customstar.h"
#include "debugprint.h"
#include "decomp.h"
#include "files.h"
#include "font.h"
#include "keyboardinput.h"
#include "mainmenubutton.h"
#include "mechchassis.h"
#include "mechvariant.h"
#include "menudata.h"
#include "menuscreen.h"
#include "messages.h"
#include "missionui.h"
#include "mousestate.h"
#include "options.h"
#include "projectarchive.h"
#include "refreshmode.h"
#include "screenfield.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "textglyph.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <string.h>

// The mech bay: the variant being edited, its engine, weapons and armor, and the fields that
// show them.

DECOMP_SIZE_ASSERT(ScreenField, 0x2c)
DECOMP_SIZE_ASSERT(EngineType, 0x0c)
DECOMP_SIZE_ASSERT(Weapon, 0x28)
DECOMP_SIZE_ASSERT(MekVariant::Item, 0x08)
DECOMP_SIZE_ASSERT(MekVariant::Armor, 0x10)
DECOMP_SIZE_ASSERT(MekVariant, 0x7a8)
DECOMP_SIZE_ASSERT(MechChassis, 0x18)
DECOMP_SIZE_ASSERT(MekLocation, 0x28)
DECOMP_SIZE_ASSERT(Equipment, 0x08)
DECOMP_SIZE_ASSERT(InternalStructure, 0x10)
DECOMP_SIZE_ASSERT(MekHeader, 0x18)
DECOMP_SIZE_ASSERT(MekItem, 0x08)

MechS32 PlayVideo(
	MechS32 p_index,
	const char* p_name,
	undefined4 p_unk0x08,
	undefined4 p_unk0x0c,
	MechU32 p_unk0x10,
	MechU32 p_unk0x14
);

// GLOBAL: MW2SHELL 0x1005c438
Equipment g_equipment[23] = {
	{5000, "MASC"},
	{5050, "Targeting Computer"},
	{5100, "ECM"},
	{5150, "Artemis IV"},
	{5200, "Beagle Active Probe"},
	{5250, "TAG"},
	{5300, "Shoulder"},
	{5350, "Upper Arm Actuator"},
	{5400, "Lower Arm Actuator"},
	{5450, "Hand Actuator"},
	{5500, "Hip"},
	{5550, "Upper Leg Actuator"},
	{5600, "Lower Leg Actuator"},
	{5650, "Foot Actuator"},
	{5700, "Sensors"},
	{5750, "Cockpit"},
	{5800, "Gyro"},
	{5850, "Engine"},
	{5900, "Life Support"},
	{6000, "Heat Sink"},
	{7000, "Jump Jet"},
	{8000, "Endo Steel"},
	{9000, "Ferro-Fibrous"},
};

// GLOBAL: MW2SHELL 0x1005c4f0
MechChar* g_locationNames[8] =
	{"Head", "Right Torso", "Center Torso", "Left Torso", "Right Arm", "Left Arm", "Right Leg", "Left Leg"};

// GLOBAL: MW2SHELL 0x1005c510
InternalStructure g_internalStructure[19] = {
	{4, 3, 1, 2},     {5, 4, 2, 3},     {6, 5, 3, 4},     {8, 6, 4, 6},     {10, 7, 5, 7},
	{11, 8, 6, 8},    {12, 10, 6, 10},  {14, 11, 7, 11},  {16, 12, 8, 12},  {18, 13, 9, 13},
	{20, 14, 10, 14}, {21, 15, 10, 15}, {22, 15, 11, 15}, {23, 16, 12, 16}, {25, 17, 13, 17},
	{27, 18, 14, 18}, {29, 19, 15, 19}, {30, 20, 16, 20}, {31, 21, 17, 21},
};

// GLOBAL: MW2SHELL 0x1005c640
MekVariant g_variant = {0};

// The variant LoadMekFile replaced.
// GLOBAL: MW2SHELL 0x1005cde8
MekVariant g_previousVariant = {0};

// GLOBAL: MW2SHELL 0x1005d590
EngineType g_engines[] = {
	{10, 50, "Omni"},      {15, 50, "GM"},          {20, 50, "Pitban"},    {25, 50, "Omni"},
	{30, 100, "Nissan"},   {35, 100, "VOX"},        {40, 100, "GM"},       {45, 100, "GM"},
	{50, 150, "DAV"},      {55, 150, "VOX"},        {60, 150, "Leenex"},   {65, 200, "Nissan"},
	{70, 200, "Omni"},     {75, 200, "GM"},         {80, 250, "VOX"},      {85, 250, "DAV"},
	{90, 300, "DAV"},      {95, 300, "Nissan"},     {100, 300, "Hermes"},  {105, 350, "DAV"},
	{110, 350, "GM"},      {115, 400, "GM"},        {120, 400, "GM"},      {125, 400, "Vlar"},
	{130, 450, "Magna"},   {135, 450, "Hermes"},    {140, 500, "Leenex"},  {145, 500, "Omni"},
	{150, 550, "GM"},      {155, 550, "GM"},        {160, 600, "LTV"},     {165, 600, "VOX"},
	{170, 600, "DAV"},     {175, 700, "Omni"},      {180, 700, "GM"},      {185, 750, "GM"},
	{190, 750, "DAV"},     {195, 800, "Nissan"},    {200, 850, "Nissan"},  {205, 850, "Vlar"},
	{210, 900, "GM"},      {215, 850, "Core Tex"},  {220, 1000, "DAV"},    {225, 1000, "VOX"},
	{230, 1050, "Leenex"}, {235, 1100, "GM"},       {240, 1150, "Pitban"}, {245, 1200, "Magna"},
	{250, 1250, "Magna"},  {255, 1300, "Strand"},   {260, 1350, "Magna"},  {265, 1400, "Vlar"},
	{270, 1450, "GM"},     {275, 1550, "Core Tex"}, {280, 1600, "VOX"},    {285, 1650, "Pitban"},
	{290, 1750, "Omni"},   {295, 1800, "GM"},       {300, 1900, "Vlar"},   {305, 1950, "GM"},
	{310, 2050, "Magna"},  {315, 2150, "GM"},       {320, 2250, "Pitban"}, {325, 2350, "VOX"},
	{330, 2400, "VOX"},    {335, 2550, "Leenex"},   {340, 2700, "VOX"},    {345, 2850, "Vlar"},
	{350, 2950, "Magna"},  {355, 3150, "LTV"},      {360, 3300, "Hermes"}, {365, 3450, "Hermes"},
	{370, 3650, "Magna"},  {375, 3850, "GM"},       {380, 4100, "GM"},     {385, 4350, "LTV"},
	{390, 4600, "Magna"},  {395, 4900, "Hermes"},   {400, 5250, "LTV"},    {-1, 0, NULL},
};

// GLOBAL: MW2SHELL 0x1005d950
Weapon g_weapons[] = {
	{6, -1, -1, 7, 14, 1000, 500, 4, 6, "LRM 20"},
	{5, -1, -1, 7, 14, 1000, 350, 2, 8, "LRM 15"},
	{4, -1, -1, 7, 14, 1000, 250, 1, 12, "LRM 10"},
	{2, -1, -1, 7, 14, 1000, 100, 1, 24, "LRM 5"},
	{4, -2, -1, 3, 6, 497, 150, 1, 15, "SRM 6"},
	{3, -2, -1, 3, 6, 497, 100, 1, 25, "SRM 4"},
	{2, -2, -1, 3, 6, 497, 50, 1, 50, "SRM 2"},
	{4, -2, -1, 4, 8, 497, 300, 2, 15, "Streak SRM-6"},
	{3, -2, -1, 4, 8, 497, 200, 1, 25, "Streak SRM-4"},
	{2, -2, -1, 4, 8, 497, 100, 1, 50, "Streak SRM-2"},
	{0, 2, -1, 1, 2, 175, 25, 1, 200, "Machine Gun"},
	{1, 15, 2, 7, 15, 1820, 1200, 6, 8, "Gauss Rifle"},
	{1, 2, 4, 10, 20, 800, 500, 3, 45, "LB 2-X AC"},
	{1, 5, 3, 8, 15, 700, 700, 4, 20, "LB 5-X AC"},
	{2, 10, -1, 6, 12, 600, 1000, 5, 10, "LB 10-X AC"},
	{6, 20, -1, 4, 8, 450, 1200, 9, 5, "LB 20-X AC"},
	{1, 2, 2, 9, 18, 700, 500, 2, 45, "Ultra AC/2"},
	{1, 5, -1, 7, 14, 600, 700, 3, 20, "Ultra AC/5"},
	{3, 10, -1, 6, 12, 500, 1000, 4, 10, "Ultra AC/10"},
	{7, 20, -1, 4, 8, 400, 1200, 8, 5, "Ultra AC/20"},
	{3, 2, -1, 1, 2, 3, 50, 1, 0, "Flamer"},
	{15, 15, -1, 7, 14, 746, 600, 2, 0, "ER PPC"},
	{12, 10, -1, 8, 15, 1019, 400, 1, 0, "ER Laser (Large)"},
	{5, 7, -1, 5, 10, 510, 100, 1, 0, "ER Laser (Medium)"},
	{2, 5, -1, 2, 4, 255, 50, 1, 0, "ER Laser (Small)"},
	{10, 10, -1, 6, 14, 815, 600, 2, 0, "Pulse Laser (Large)"},
	{4, 7, -1, 4, 8, 408, 200, 1, 0, "Pulse Laser (Medium)"},
	{2, 3, -1, 2, 4, 204, 100, 1, 0, "Pulse Laser (Small)"},
	{0, 0, -1, 4, 8, 12, 200, 1, 0, "Narc Missile Beacon"},
	{1, 0, -1, -1, -1, -1, 50, 1, 0, "Anti-Missile System"},
	{0, 0, -1, -1, -1, -1, 0, 1, 0, "Nuke"},
};

// The field table of the component shown on the right, NULL for none.
// GLOBAL: MW2SHELL 0x1005de28
ScreenField* g_componentFields = NULL;

// The screen's own fields: the bay's or the customize screen's.
// GLOBAL: MW2SHELL 0x1005de2c
ScreenField* g_screenFields = NULL;

// The location UpdateArmor takes the next point of armor from.
// GLOBAL: MW2SHELL 0x1005de30
MechS32 g_armorTrimLocation = 0;

// Where the location map's frame of each location goes.
// GLOBAL: MW2SHELL 0x1005de38
MechS32 g_locationMapLefts[8] = {75, 46, 86, 126, 10, 162, 39, 109};

// GLOBAL: MW2SHELL 0x1005de58
MechS32 g_locationMapTops[8] = {171, 206, 206, 206, 204, 204, 310, 310};

// GLOBAL: MW2SHELL 0x1005de78
AudioSample* g_locationSound = NULL;

// GLOBAL: MW2SHELL 0x10061560
MechChassis g_mechChassis[] = {
	{"ds", "frm", "firemoth", "Firemoth", 20, 85},
	{"kf", "ktf", "kitfox", "Kit Fox", 30, 89},
	{"jn", "jnr", "jenner", "Jenner IIC", 35, 88},
	{"bh", "nva", "nova", "Nova", 50, 92},
	{"sc", "stm", "strmcrow", "Stormcrow", 55, 94},
	{"md", "mdg", "maddog", "Mad Dog", 60, 90},
	{"lo", "hlb", "hellbrgr", "Hellbringer", 65, 87},
	{"rf", "rfl", "rifleman", "Rifleman IIC", 65, 93},
	{"su", "smn", "summoner", "Summoner", 70, 95},
	{"mc", "tbr", "timbrwlf", "Timber Wolf", 75, 96},
	{"mw", "grg", "gargoyle", "Gargoyle", 80, 86},
	{"wh", "whm", "warhammr", "Warhammer IIC", 80, 97},
	{"mr", "mrd", "marauder", "Marauder IIC ", 85, 91},
	{"ms", "whk", "warhawk", "Warhawk", 85, 98},
	{"da", "drw", "direwolf", "Dire Wolf", 100, 83},
	{"el", "ele", "elementl", "Elemental", 100, 84},
	{"ta", "tar", "tarantul", "Tarantula", 100, -1},
	{"bm", "btm", "btllmstr", "Battle Master IIC", 100, 99},
	{NULL, NULL, NULL, NULL, 0, 0},
};

// The chassis on offer: 15, and up to 18 in a trial (GetChassisCount).
// GLOBAL: MW2SHELL 0x10061728
MechS32 g_chassisCount = 15;

// The chassis video of each campaign's bay, with the chassis code.
// GLOBAL: MW2SHELL 0x10061730
MechChar g_wolfChassisVideo[] = "awomp%s";

// GLOBAL: MW2SHELL 0x10061738
MechChar g_jadeFalconChassisVideo[] = "ajfmp%s";

// GLOBAL: MW2SHELL 0x10061740
MechChar g_trialChassisVideo[] = "aiamp%s";

// GLOBAL: MW2SHELL 0x10061748
MechChar* g_chassisVideoFormat = g_wolfChassisVideo;

// GLOBAL: MW2SHELL 0x1006176c
MechS32 g_selectedChassis = 7;

// GLOBAL: MW2SHELL 0x10061770
AudioSample* g_chassisNameSound = NULL;

// Set by the star screen when it opens the bay to change one of the star's mechs.
// GLOBAL: MW2SHELL 0x10061774
MechS32 g_pickStarMech = 0;

// GLOBAL: MW2SHELL 0x10061778
ButtonMenu* g_mechBayMenu = NULL;

// GLOBAL: MW2SHELL 0x1006177c
AudioSample* g_mechBayAmbience = NULL;

// The wParam the screen was opened with.
// GLOBAL: MW2SHELL 0x10061780
MechS32 g_mechBayWParam = c_msgScreenFrame;

// Set to 1 when the mech bay closes; nothing reads it.
// GLOBAL: MW2SHELL 0x1006178c
MechS32 g_unk0x1006178c = 1;

// GLOBAL: MW2SHELL 0x10079a98
undefined4 g_chassisVideoLeft;

// GLOBAL: MW2SHELL 0x10079a9c
undefined4 g_chassisVideoTop;

// g_pickStarMech as the bay was opened: ACCEPT MECH checks the star's tonnage, and the other
// screens are out of reach.
// GLOBAL: MW2SHELL 0x10079aa0
MechS32 g_pickingStarMech;

// GLOBAL: MW2SHELL 0x10079aa8
MechChar g_userMekName[0x20];

// GLOBAL: MW2SHELL 0x10079ac8
AudioSample* g_acceptSound;

// The pilot's callsign and the star's maximum weight.
// GLOBAL: MW2SHELL 0x10079ad0
MechChar g_callsignLine[0x80];

// GLOBAL: MW2SHELL 0x10079b50
MechChar g_tempBuffer[0x100];

// The image of a .mek file, besides g_mekLocations: its header, the ammunition and weapons
// and the variant name.
// GLOBAL: MW2SHELL 0x10079c50
MekItem g_mekAmmo[25];

// GLOBAL: MW2SHELL 0x10079d18
AudioSample* g_variantSound;

// GLOBAL: MW2SHELL 0x10079d20
MekHeader g_mekHeader;

// GLOBAL: MW2SHELL 0x10079d38
MechChar g_chassisVideoName[0x10];

// GLOBAL: MW2SHELL 0x10079d48
MechChar g_mekVariantName[0x32];

// The chassis' variant files: its standard ones in 0-99, the user's in 100-199.
// GLOBAL: MW2SHELL 0x10079d80
MechChar g_variantFiles[200][13];

// GLOBAL: MW2SHELL 0x1007a7a8
MekItem g_mekWeapons[10];

// DrawMechBay's p_wParam. Never read.
// GLOBAL: MW2SHELL 0x1007a7f8
MechS32 g_mechBayMessage;

// GLOBAL: MW2SHELL 0x1007a800
MechChar g_variantFileName[0x20];

// GLOBAL: MW2SHELL 0x1007a820
undefined g_mekFileBuffer[0x800];

// GLOBAL: MW2SHELL 0x1007c820
MekLocation g_mekLocations[8];

// The color maps of the fields: over the maximum mass, clickable or highlighted, and plain
// text.
// GLOBAL: MW2SHELL 0x1007c960
undefined g_warningColors[0x100];

// GLOBAL: MW2SHELL 0x1007ca60
undefined g_activeColors[0x100];

// GLOBAL: MW2SHELL 0x1007cb60
undefined g_textColors[0x100];

// The path of the .mek file being saved.
// GLOBAL: MW2SHELL 0x1007cc60
MechChar g_mekPath[0x20];

// The variant shown, an index into g_variantFiles.
// GLOBAL: MW2SHELL 0x1007cc80
MechS32 g_selectedVariant;

// FUNCTION: MW2SHELL 0x10007850
MechS32 RoundUpTons(MechS32 p_value)
{
	MechS32 remainder;

	remainder = p_value % 100;
	return remainder ? p_value + 100 - remainder : p_value;
}

// FUNCTION: MW2SHELL 0x1000788c
MechS32 RoundDownTons(MechS32 p_value)
{
	return p_value - p_value % 100;
}

// FUNCTION: MW2SHELL 0x100078ae
MechS32 RoundTons(MechS32 p_value)
{
	return RoundDownTons(p_value + 50);
}

// Stack-slot permutation: top and height.
// FUNCTION: MW2SHELL 0x100078cd
void ShowFields(ScreenField* p_tabs)
{
	MechS32 top;
	MechS32 height;

	top = 0;
	height = 0;
	if (p_tabs == NULL) {
		return;
	}

	while (p_tabs->m_left != -1) {
		if (p_tabs->m_top < 0) {
			p_tabs->m_top = ((p_tabs->m_top & 0xff0) >> 4) * height + (p_tabs->m_top & 0xf) + top;
		}

		if (p_tabs->m_draw != NULL) {
			p_tabs->m_glyph = p_tabs->m_draw(p_tabs);
		}
		else {
			p_tabs->m_glyph = NULL;
		}

		if (p_tabs->m_glyph != NULL) {
			if (p_tabs->m_width == -1) {
				p_tabs->m_width = p_tabs->m_glyph->m_width;
			}
			if (p_tabs->m_height == -1) {
				p_tabs->m_height = p_tabs->m_glyph->m_height;
			}
		}

		if (p_tabs->m_width == -1) {
			p_tabs->m_width = 0;
		}
		if (p_tabs->m_height == -1) {
			p_tabs->m_height = height;
		}

		top = p_tabs->m_top;
		height = p_tabs->m_height;
		p_tabs++;
	}
}

// FUNCTION: MW2SHELL 0x100079f8
void RedrawFields(ScreenField* p_tabs)
{
	ScreenField* tab;

	if (p_tabs == NULL) {
		return;
	}

	for (tab = p_tabs; tab->m_left != -1; tab++) {
		if (tab->m_glyph != NULL) {
			delete tab->m_glyph;
			tab->m_glyph = NULL;
		}
	}

	for (tab = p_tabs; tab->m_left != -1; tab++) {
		if (tab->m_draw != NULL) {
			tab->m_glyph = tab->m_draw(tab);
		}
	}
}

// FUNCTION: MW2SHELL 0x10007ac8
void HideFields(ScreenField* p_tabs)
{
	if (p_tabs == NULL) {
		return;
	}

	while (p_tabs->m_left != -1) {
		if (p_tabs->m_glyph != NULL) {
			delete p_tabs->m_glyph;
			p_tabs->m_glyph = NULL;
		}

		p_tabs++;
	}
}

// FUNCTION: MW2SHELL 0x10007b4d
MechS32 RemoveCriticals(MechS32 p_id)
{
	MechS32 count;
	MechS32 j;
	MechS32 i;

	count = 0;
	if (p_id <= 0) {
		return count;
	}

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 12; j++) {
			if (g_variant.m_criticals[i][j] == p_id) {
				g_variant.m_criticals[i][j] = 0;
				count++;
			}
		}
	}

	return count;
}

// Array index order: the original loads i before p_location in m_criticals[p_location][i]. The
// order follows the unit's symbol table: renaming the unit's globals flipped it here and fixed it
// in RemoveCriticals.
// FUNCTION: MW2SHELL 0x10007bee
MechS32 PlaceCriticals(MechS32 p_location, MechS32 p_id, MechS32 p_count)
{
	MechS32 i;

	if (p_id <= 0) {
		return FALSE;
	}

	for (i = 0; p_count && i < 12; i++) {
		if (g_variant.m_criticals[p_location][i] == 0) {
			g_variant.m_criticals[p_location][i] = p_id;
			p_count--;
		}
	}

	if (p_count) {
		for (i = 0; i < 12; i++) {
			if (g_variant.m_criticals[p_location][i] == p_id) {
				g_variant.m_criticals[p_location][i] = 0;
			}
		}

		return FALSE;
	}

	return TRUE;
}

// FUNCTION: MW2SHELL 0x10007cd4
void RemoveUnassigned(MechS32 p_id)
{
	MechS32 j;
	MechS32 i;

	if (p_id <= 0) {
		return;
	}

	j = 0;
	for (i = 0; i < 78; i++) {
		if (g_variant.m_unassigned[i].m_id == p_id) {
			g_variant.m_unassigned[i].m_id = -1;
			g_variant.m_unassignedCount--;
		}

		if (g_variant.m_unassigned[i].m_id != -1) {
			if (g_variant.m_unassigned[j].m_id == -1) {
				g_variant.m_unassigned[j] = g_variant.m_unassigned[i];
				g_variant.m_unassigned[i].m_id = -1;
			}
			j++;
		}
	}
}

// FUNCTION: MW2SHELL 0x10007d9e
void AddUnassigned(MechS32 p_id, MechS32 p_unk0x04)
{
	if (p_id <= 0) {
		return;
	}

	if (g_variant.m_unassignedCount < 78) {
		g_variant.m_unassigned[g_variant.m_unassignedCount].m_id = p_id;
		g_variant.m_unassigned[g_variant.m_unassignedCount].m_criticals = p_unk0x04;
		g_variant.m_unassignedCount++;
	}
}

// FUNCTION: MW2SHELL 0x10007df0
void UnassignItem(MechS32 p_id)
{
	MechS32 count;

	if (p_id <= 0) {
		return;
	}

	count = RemoveCriticals(p_id);
	if (count) {
		AddUnassigned(p_id, count);
	}
}

// FUNCTION: MW2SHELL 0x10007e3b
MechS32 AssignItem(MechS32 p_location, MechS32 p_id, MechS32 p_count)
{
	if (p_id <= 0) {
		return FALSE;
	}

	if (PlaceCriticals(p_location, p_id, p_count)) {
		RemoveUnassigned(p_id);
		return TRUE;
	}

	return FALSE;
}

// Stack-slot permutation: id, group, index, i and j. m_criticals[i][j] also loads the base before
// the index, the reverse of the original.
// FUNCTION: MW2SHELL 0x10007e90
void DeleteItem(MechS32 p_id)
{
	MechS32 id;
	MechS32 group;
	MechS32 index;
	MechS32 i;
	MechS32 j;

	group = p_id / 100;
	index = p_id % 100;
	if (p_id <= 0) {
		return;
	}

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 12; j++) {
			id = g_variant.m_criticals[i][j];
			if (id == p_id) {
				id = 0;
			}
			else if (id > 0 && (id < 5000 || id >= 10000)) {
				if (id / 100 == group && id % 100 > index) {
					id--;
				}
				else if (p_id < 5000 && id >= 10000) {
					if ((id - 10000) / 100 == p_id) {
						id = 0;
					}
					else if ((id - 10000) / 10000 == group && id / 100 % 100 > index) {
						id -= 100;
					}
				}
			}

			g_variant.m_criticals[i][j] = id;
		}
	}

	for (i = 0, j = 0; i < 78; i++) {
		id = g_variant.m_unassigned[i].m_id;
		if (id == p_id) {
			id = -1;
			g_variant.m_unassignedCount--;
		}
		else if (id > 0 && (id < 5000 || id >= 10000)) {
			if (id / 100 == group && id % 100 > index) {
				id--;
			}
			else if (p_id < 5000 && id >= 10000) {
				if ((id - 10000) / 100 == p_id) {
					id = -1;
					g_variant.m_unassignedCount--;
				}
				else if ((id - 10000) / 10000 == group && id / 100 % 100 > index) {
					id -= 100;
				}
			}
		}

		g_variant.m_unassigned[i].m_id = id;
		if (id != -1) {
			g_variant.m_unassigned[j] = g_variant.m_unassigned[i];
			j++;
		}
	}

	while (j < 78) {
		g_variant.m_unassigned[j].m_id = -1;
		j++;
	}
}

// FUNCTION: MW2SHELL 0x1000819f
MechS32 AddAmmo(MechS32 p_id)
{
	MechS32 i;

	p_id -= p_id % 100;
	p_id++;
	for (i = 0; i < 25; i++) {
		if (g_variant.m_ammo[i] == -1) {
			if (p_id % 100 > 10) {
				return 0;
			}

			g_variant.m_ammo[i] = p_id;
			return p_id;
		}

		if (g_variant.m_ammo[i] / 100 == p_id / 100) {
			p_id++;
		}
	}

	return 0;
}

// Stack-slot permutation: count and i.
// FUNCTION: MW2SHELL 0x10008254
MechS32 CountAmmo(MechS32 p_id)
{
	MechS32 count;
	MechS32 i;

	count = 0;
	p_id = (p_id * 100 + 10000) / 100;
	for (i = 0; i < 25; i++) {
		if (g_variant.m_ammo[i] == -1) {
			break;
		}

		if (g_variant.m_ammo[i] / 100 == p_id) {
			count++;
		}
	}

	return count;
}

// FUNCTION: MW2SHELL 0x100082de
void RemoveAmmo(MechS32 p_id)
{
	MechS32 j;
	MechS32 i;

	j = 0;
	for (i = 0; i < 25; i++) {
		if (g_variant.m_ammo[i] == p_id) {
			g_variant.m_ammo[i] = -1;
		}

		if (g_variant.m_ammo[i] != -1) {
			if (g_variant.m_ammo[j] == -1) {
				g_variant.m_ammo[j] = g_variant.m_ammo[i];
				g_variant.m_ammo[i] = -1;
			}

			if (g_variant.m_ammo[j] / 100 == p_id / 100 && g_variant.m_ammo[j] % 100 > p_id % 100) {
				g_variant.m_ammo[j]--;
			}

			j++;
		}
	}
}

// FUNCTION: MW2SHELL 0x100083d6
MechS32 AddWeapon(MechS32 p_id)
{
	MechS32 i;

	p_id -= p_id % 100;
	p_id++;
	for (i = 0; i < 10; i++) {
		if (g_variant.m_weapons[i] == -1) {
			g_variant.m_weapons[i] = p_id;
			return p_id;
		}

		if (g_variant.m_weapons[i] / 100 == p_id / 100) {
			p_id++;
		}
	}

	return 0;
}

// FUNCTION: MW2SHELL 0x10008470
void RemoveWeapon(MechS32 p_id)
{
	MechS32 j;
	MechS32 i;

	j = 0;
	for (i = 0; i < 10; i++) {
		if (g_variant.m_weapons[i] == p_id) {
			g_variant.m_weapons[i] = -1;
		}

		if (g_variant.m_weapons[i] != -1) {
			if (g_variant.m_weapons[j] == -1) {
				g_variant.m_weapons[j] = g_variant.m_weapons[i];
				g_variant.m_weapons[i] = -1;
			}

			if (g_variant.m_weapons[j] / 100 == p_id / 100 && g_variant.m_weapons[j] % 100 > p_id % 100) {
				g_variant.m_weapons[j]--;
			}

			j++;
		}
	}

	p_id = p_id * 100 + 10001;
	j = 0;
	for (i = 0; i < 25; i++) {
		if (g_variant.m_ammo[i] / 100 == p_id / 100) {
			g_variant.m_ammo[i] = -1;
		}

		if (g_variant.m_ammo[i] != -1) {
			if (g_variant.m_ammo[j] == -1) {
				g_variant.m_ammo[j] = g_variant.m_ammo[i];
				g_variant.m_ammo[i] = -1;
			}

			if (g_variant.m_ammo[j] / 10000 == p_id / 10000 && g_variant.m_ammo[j] / 100 % 100 > p_id / 100 % 100) {
				g_variant.m_ammo[j] -= 100;
			}

			j++;
		}
	}
}

// Operand order: the original adds m_engineMass last in the m_usedMass sum; it follows the unit's
// symbol table.
// FUNCTION: MW2SHELL 0x10008686
void UpdateMass()
{
	g_variant.m_gyroMass = RoundUpTons((MechS32) (g_variant.m_engineRating / 100.0 * 100.0));
	g_variant.m_internalMass = g_variant.m_maxMass / (g_variant.m_endoSteel ? 20 : 10);
	g_variant.m_cockpitMass = 300;
	g_variant.m_usedMass = g_variant.m_jumpJetMass + g_variant.m_heatSinkMass + g_variant.m_equipmentMass +
						   g_variant.m_weaponMass + g_variant.m_armorMass + g_variant.m_gyroMass +
						   g_variant.m_internalMass + g_variant.m_cockpitMass + g_variant.m_ammoMass +
						   g_variant.m_engineMass;
	g_variant.m_freeMass = g_variant.m_maxMass - g_variant.m_usedMass;
}

// FUNCTION: MW2SHELL 0x10008739
void UpdateSpeed()
{
	g_variant.m_walkingSpeed = g_variant.m_engineRating * 100 / g_variant.m_maxMass;
	g_variant.m_runningSpeed = (g_variant.m_walkingSpeed * 3 + 2) / 2;
}

// Empty. UpdateStats calls it after UpdateSpeed, so it is probably an update step whose body
// was removed; nothing says which.
// FUNCTION: MW2SHELL 0x10008776
void FUN_10008776()
{
}

// FUNCTION: MW2SHELL 0x10008786
void UpdateHeatSinks()
{
	MechS32 count;

	count = g_variant.m_externalHeatSinks;
	g_variant.m_externalHeatSinks = g_variant.m_heatSinkMass / 100 + 10 - g_variant.m_engineRating / 25;
	if (g_variant.m_externalHeatSinks < 0) {
		g_variant.m_externalHeatSinks = 0;
	}

	if (count >= 0 && g_variant.m_externalHeatSinks != count) {
		while (g_variant.m_externalHeatSinks < count) {
			DeleteItem(count + 6000);
			count--;
		}

		while (count < g_variant.m_externalHeatSinks) {
			count++;
			AddUnassigned(count + 6000, g_variant.m_heatSinkType);
		}
	}
}

// FUNCTION: MW2SHELL 0x1000884c
void UpdateEngine()
{
	g_variant.m_engineRating = g_engines[g_variant.m_engine % 10000].m_rating;
	g_variant.m_engineMass = g_engines[g_variant.m_engine % 10000].m_weight;
	if (g_variant.m_engine >= 10000) {
		g_variant.m_engineMass /= 2;
	}

	if (g_variant.m_engine >= 10000) {
		g_variant.m_unk0x218 = 7;
	}
	else {
		g_variant.m_unk0x218 = 0;
	}

	g_variant.m_unk0x21c = g_variant.m_maxMass - g_variant.m_engineMass;
}

// FUNCTION: MW2SHELL 0x100088ec
void UpdateJumpJets()
{
	UpdateSpeed();
	while (g_variant.m_jumpJets > g_variant.m_walkingSpeed) {
		DeleteItem(g_variant.m_jumpJets + 7000);
		g_variant.m_jumpJets--;
	}

	if (g_variant.m_maxMass <= 5500) {
		g_variant.m_jumpJetUnitMass = 50;
	}
	else if (g_variant.m_maxMass <= 8500) {
		g_variant.m_jumpJetUnitMass = 100;
	}
	else {
		g_variant.m_jumpJetUnitMass = 200;
	}

	g_variant.m_jumpJetMass = g_variant.m_jumpJets * g_variant.m_jumpJetUnitMass;
}

// FUNCTION: MW2SHELL 0x10008989
void UpdateStats()
{
	UpdateMass();
	UpdateSpeed();
	FUN_10008776();
}

// FUNCTION: MW2SHELL 0x100089a8
void UpdateWeaponMass()
{
	MechS32 i;

	g_variant.m_weaponMass = 0;
	for (i = 0; i < 10; i++) {
		if (g_variant.m_weapons[i] != -1) {
			g_variant.m_weaponMass += g_weapons[g_variant.m_weapons[i] / 100].m_mass;
		}
	}
}

// FUNCTION: MW2SHELL 0x10008a16
void UpdateArmor()
{
	MechS32 i;

	if (!g_variant.m_ferroFibrous) {
		g_variant.m_armorFactor = (g_variant.m_armorMass * 16 + 49) / 100;
	}
	else {
		g_variant.m_armorFactor = ((MechS32) (g_variant.m_armorMass * 16 * 1.2) + 49) / 100;
	}

	g_variant.m_armorAllocated = 0;
	for (i = 0; i < 8; i++) {
		g_variant.m_armorAllocated += g_variant.m_armor[i].m_front;
		if (g_variant.m_armor[i].m_rear >= 0) {
			g_variant.m_armorAllocated += g_variant.m_armor[i].m_rear;
		}
	}

	while (g_variant.m_armorAllocated > g_variant.m_armorFactor) {
		if (g_variant.m_armor[g_armorTrimLocation].m_front > 0) {
			g_variant.m_armor[g_armorTrimLocation].m_front--;
			g_variant.m_armorAllocated--;
		}

		if (g_variant.m_armorAllocated <= g_variant.m_armorFactor) {
			break;
		}

		if (g_variant.m_armor[g_armorTrimLocation].m_rear > 0) {
			g_variant.m_armor[g_armorTrimLocation].m_rear--;
			g_variant.m_armorAllocated--;
		}

		g_armorTrimLocation++;
		if (g_armorTrimLocation >= 8) {
			g_armorTrimLocation = 0;
		}
	}
}

// FUNCTION: MW2SHELL 0x10008b76
void UpdateAmmoMass()
{
	MechS32 i;

	g_variant.m_ammoMass = 0;
	for (i = 0; i < 25; i++) {
		if (g_variant.m_ammo[i] == -1) {
			break;
		}

		g_variant.m_ammoMass += 100;
	}
}

// FUNCTION: MW2SHELL 0x10008bce
TextGlyph* DrawLocationMap(ScreenField* p_tab)
{
	// An empty test of the unused parameter: the original compiles it to a cmp with a zero-length je.
	if (p_tab) {
	}

	SetVideoFrame(12, g_variant.m_selectedLocation);
	MoveVideo(
		12,
		g_locationMapLefts[g_variant.m_selectedLocation] + 0xd8,
		g_locationMapTops[g_variant.m_selectedLocation] + 0x34
	);
	ShowVideo(12);
	return NULL;
}

// FUNCTION: MW2SHELL 0x10008c30
TextGlyph* DrawMass(ScreenField* p_tab)
{
	MechS32 value;

	value = *(MechS32*) p_tab->m_data;
	sprintf(g_tempBuffer, "%d.%d%d T", value / 100, value / 10 % 10, value % 10);
	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x10008cb0
TextGlyph* DrawUsedMass(ScreenField* p_tab)
{
	MechS32 value;
	undefined* colors;

	value = *(MechS32*) p_tab->m_data;
	colors = p_tab->m_colors;
	if (value > g_variant.m_maxMass) {
		colors = g_warningColors;
	}

	sprintf(g_tempBuffer, "%d.%d%d T", value / 100, value / 10 % 10, value % 10);
	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, colors);
}

// FUNCTION: MW2SHELL 0x10008dcc
TextGlyph* DrawEngineRating(ScreenField* p_tab)
{
	MechS32 value;

	value = *(MechS32*) p_tab->m_data;
	sprintf(g_tempBuffer, value >= 10000 ? "%dXL" : "%d", g_engines[value % 10000].m_rating);
	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x10008e4f
TextGlyph* DrawEngineType(ScreenField* p_tab)
{
	MechS32 value;

	value = *(MechS32*) p_tab->m_data;
	return g_defaultFont
		->AddOverlayText(p_tab->m_left, p_tab->m_top, (MechChar*) (value >= 10000 ? "XL" : "Std"), p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x10008eaa
TextGlyph* DrawEngineMaker(ScreenField* p_tab)
{
	MechS32 value;

	value = *(MechS32*) p_tab->m_data;
	sprintf(g_tempBuffer, "%s", g_engines[value % 10000].m_name);
	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x10008f14
TextGlyph* DrawSpeed(ScreenField* p_tab)
{
	MechS32 value;

	value = *(MechS32*) p_tab->m_data;
	sprintf(g_tempBuffer, "%1.1f kph", value * 10.8);
	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x10008fdd
TextGlyph* DrawHeatSinkCount(ScreenField* p_tab)
{
	MechS32 value;
	MechS32 count;

	value = *(MechS32*) p_tab->m_data;
	count = (g_variant.m_heatSinkMass + 50) / 100 + 10;
	if (value == 1) {
		sprintf(g_tempBuffer, "%d", count);
	}
	else {
		sprintf(g_tempBuffer, "%d (%d)", count, count * 2);
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x10009076
TextGlyph* DrawHeatSinkType(ScreenField* p_tab)
{
	MechS32 value;

	value = *(MechS32*) p_tab->m_data;
	return g_defaultFont
		->AddOverlayText(p_tab->m_left, p_tab->m_top, (MechChar*) (value == 1 ? "Single" : "Double"), p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x100090ce
TextGlyph* DrawNumber(ScreenField* p_tab)
{
	MechS32 value;

	value = *(MechS32*) p_tab->m_data;
	sprintf(g_tempBuffer, "%d", value);
	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x1000918c
TextGlyph* DrawLabel(ScreenField* p_tab)
{
	MechChar* text;

	text = (MechChar*) p_tab->m_data;
	if (text == NULL) {
		return NULL;
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, text, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x100091dc
TextGlyph* DrawTitle(ScreenField* p_tab)
{
	MechChar* text;

	text = (MechChar*) p_tab->m_data;
	if (text == NULL) {
		return NULL;
	}

	return g_titleFont->AddOverlayText(p_tab->m_left, p_tab->m_top, text, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x1000922c
TextGlyph* DrawInternalType(ScreenField* p_tab)
{
	MechS32 value;

	value = *(MechS32*) p_tab->m_data;
	return g_defaultFont
		->AddOverlayText(p_tab->m_left, p_tab->m_top, (MechChar*) (value ? "Endo-S" : "Std"), p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x10009284
TextGlyph* DrawArmorType(ScreenField* p_tab)
{
	MechS32 value;

	value = *(MechS32*) p_tab->m_data;
	return g_defaultFont
		->AddOverlayText(p_tab->m_left, p_tab->m_top, (MechChar*) (value ? "Ferro-F" : "Std"), p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x100092dc
TextGlyph* DrawLocationArmor(ScreenField* p_tab)
{
	MekVariant::Armor* armor;

	armor = &g_variant.m_armor[g_variant.m_selectedLocation];
	if (p_tab->m_data) {
		if (armor->m_rear >= 0) {
			sprintf(g_tempBuffer, "~%d", armor->m_rear);
		}
		else {
			strcpy(g_tempBuffer, "~--");
		}
	}
	else {
		sprintf(g_tempBuffer, "~%d", armor->m_front);
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x100093a7
TextGlyph* DrawArmorAllocation(ScreenField* p_tab)
{
	MekVariant::Armor* armor;

	armor = &g_variant.m_armor[MECH_PTR_TO_S32(p_tab->m_data)];
	if (armor->m_rear >= 0) {
		sprintf(g_tempBuffer, "~%d/%d", armor->m_front, armor->m_rear);
	}
	else {
		sprintf(g_tempBuffer, "~%d", armor->m_front);
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x1000943f
TextGlyph* DrawLocationMaxArmor(ScreenField* p_tab)
{
	MechS32 location;

	location = MECH_PTR_TO_S32(p_tab->m_data);
	if (location == -1) {
		return NULL;
	}

	sprintf(g_tempBuffer, "%s (%d)", g_locationNames[location], g_variant.m_armor[location].m_maxArmor);
	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x100094ba
TextGlyph* DrawSelectedLocation(ScreenField* p_tab)
{
	MechS32 location;

	location = *(MechS32*) p_tab->m_data;
	if (location == -1) {
		return NULL;
	}

	sprintf(g_tempBuffer, "%s (%d)", g_locationNames[location], g_variant.m_armor[location].m_maxArmor);
	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x10009537
TextGlyph* DrawWeaponEntry(ScreenField* p_tab)
{
	MechS32 value;
	MechS32 count;
	undefined* colors;

	value = *(MechS32*) p_tab->m_data;
	colors = p_tab->m_colors;
	if (value == -1) {
		strcpy(g_tempBuffer, "-");
	}
	else {
		if (g_weapons[value / 100].m_ammoPerTon) {
			count = CountAmmo(value);
			sprintf(
				g_tempBuffer,
				"%s #%d (ammo %dT/%d)",
				g_weapons[value / 100].m_name,
				value % 100,
				count,
				g_weapons[value / 100].m_ammoPerTon * count
			);
		}
		else {
			sprintf(g_tempBuffer, "%s #%d", g_weapons[value / 100].m_name, value % 100);
		}

		if (g_variant.m_selectedWeapon == value) {
			colors = g_activeColors;
		}
	}

	if (p_tab->m_left >= 0x1b4) {
		return g_defaultFont->AddText(p_tab->m_left, p_tab->m_top, g_tempBuffer, colors);
	}
	else {
		return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, colors);
	}
}

// FUNCTION: MW2SHELL 0x100096bd
TextGlyph* DrawWeaponTableEntry(ScreenField* p_tab)
{
	MechS32 index;
	undefined* colors;

	index = MECH_PTR_TO_S32(p_tab->m_data);
	colors = p_tab->m_colors;
	if (index == -1) {
		return NULL;
	}

	if (g_variant.m_selectedWeapon == index * 100) {
		colors = g_activeColors;
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_weapons[index].m_name, colors);
}

// Draws one statistic (p_tab->m_data) of the highlighted weapon.
// FUNCTION: MW2SHELL 0x100097a0
TextGlyph* DrawWeaponInfo(ScreenField* p_tab)
{
	MechS32 weapon;

	if (g_variant.m_selectedWeapon == -1) {
		return NULL;
	}

	weapon = g_variant.m_selectedWeapon / 100;
	switch (MECH_PTR_TO_S32(p_tab->m_data)) {
	case 0:
		if (g_variant.m_selectedWeapon % 100) {
			sprintf(g_tempBuffer, "%s #%d", g_weapons[weapon].m_name, g_variant.m_selectedWeapon % 100);
		}
		else {
			sprintf(g_tempBuffer, "%s", g_weapons[weapon].m_name);
		}
		break;
	case 1:
		sprintf(g_tempBuffer, "%d", g_weapons[weapon].m_heat);
		break;
	case 2:
		if (g_weapons[weapon].m_damage == 0) {
			strcpy(g_tempBuffer, "-");
		}
		else if (g_weapons[weapon].m_damage < 0) {
			sprintf(g_tempBuffer, "%d/missile", -g_weapons[weapon].m_damage);
		}
		else {
			sprintf(g_tempBuffer, "%d", g_weapons[weapon].m_damage);
		}
		break;
	case 3:
		if (g_weapons[weapon].m_minimumRange >= 0) {
			sprintf(g_tempBuffer, "%d", g_weapons[weapon].m_minimumRange);
		}
		else {
			strcpy(g_tempBuffer, "-");
		}
		break;
	case 4:
		if (g_weapons[weapon].m_shortRange == -1) {
			strcpy(g_tempBuffer, "-");
		}
		else if (g_weapons[weapon].m_shortRange == 1) {
			strcpy(g_tempBuffer, "1");
		}
		else {
			sprintf(g_tempBuffer, "1-%d", g_weapons[weapon].m_shortRange);
		}
		break;
	case 5:
		if (g_weapons[weapon].m_mediumRange == -1) {
			strcpy(g_tempBuffer, "-");
		}
		else if (g_weapons[weapon].m_shortRange + 1 == g_weapons[weapon].m_mediumRange) {
			sprintf(g_tempBuffer, "%d", g_weapons[weapon].m_mediumRange);
		}
		else {
			sprintf(g_tempBuffer, "%d-%d", g_weapons[weapon].m_shortRange + 1, g_weapons[weapon].m_mediumRange);
		}
		break;
	case 6:
		if (g_weapons[weapon].m_range == -1) {
			strcpy(g_tempBuffer, "-");
		}
		else {
			sprintf(g_tempBuffer, "%d", g_weapons[weapon].m_range);
		}
		break;
	case 7:
		if (g_weapons[weapon].m_mass % 10) {
			sprintf(
				g_tempBuffer,
				"%d.%d%dT",
				g_weapons[weapon].m_mass / 100,
				g_weapons[weapon].m_mass / 10 % 10,
				g_weapons[weapon].m_mass % 10
			);
		}
		else if (g_weapons[weapon].m_mass % 100) {
			sprintf(g_tempBuffer, "%d.%dT", g_weapons[weapon].m_mass / 100, g_weapons[weapon].m_mass / 10 % 10);
		}
		else {
			sprintf(g_tempBuffer, "%dT", g_weapons[weapon].m_mass / 100);
		}
		break;
	case 8:
		sprintf(g_tempBuffer, "%d", g_weapons[weapon].m_criticals);
		break;
	case 9:
		if (g_weapons[weapon].m_ammoPerTon) {
			sprintf(g_tempBuffer, "%d", g_weapons[weapon].m_ammoPerTon);
		}
		else {
			strcpy(g_tempBuffer, "-");
		}
		break;
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x10009d50
TextGlyph* DrawLocationName(ScreenField* p_tab)
{
	MechS32 location;

	location = *(MechS32*) p_tab->m_data;
	if (location == -1) {
		return NULL;
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_locationNames[location], p_tab->m_colors);
}

// Draws the item in one critical slot (p_tab->m_data) of the selected location: a weapon, a
// piece of equipment or ammunition (ids from 10000 up).
// Not 100%: the stack slots of id and i are permuted.
// FUNCTION: MW2SHELL 0x10009da9
TextGlyph* DrawCritical(ScreenField* p_tab)
{
	MechS32 id;
	MechS32 i;

	if (g_variant.m_selectedLocation == -1) {
		return NULL;
	}

	id = g_variant.m_criticals[g_variant.m_selectedLocation][MECH_PTR_TO_S32(p_tab->m_data)];
	if (id == -1) {
		return NULL;
	}

	id &= ~0x80000000;
	if (id == 0) {
		strcpy(g_tempBuffer, "-");
	}
	else if (id < 5000) {
		sprintf(g_tempBuffer, "%s #%d", g_weapons[id / 100].m_name, id % 100);
	}
	else if (id < 10000) {
		for (i = 22; i >= 0; i--) {
			if (g_equipment[i].m_id / 50 == id / 50) {
				sprintf(g_tempBuffer, "%s", g_equipment[i].m_name);
				break;
			}
		}
		if (i < 0) {
			sprintf(g_tempBuffer, "BAD CRITICAL %d", id);
		}
	}
	else {
		id -= 10000;
		sprintf(g_tempBuffer, "Ammo (%s #%d) #%d", g_weapons[id / 10000].m_name, id / 100 % 100, id % 100);
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// Draws one entry (p_tab->m_data) of the items list with its count, like DrawCritical.
// Not 100%: the stack slots of id, count and i are permuted.
// FUNCTION: MW2SHELL 0x10009f92
TextGlyph* DrawUnassigned(ScreenField* p_tab)
{
	MechS32 id;
	MechS32 count;
	MechS32 i;

	if (g_variant.m_selectedLocation == -1) {
		return NULL;
	}

	id = g_variant.m_unassigned[MECH_PTR_TO_S32(p_tab->m_data)].m_id;
	count = g_variant.m_unassigned[MECH_PTR_TO_S32(p_tab->m_data)].m_criticals;
	if (id == -1 || id == 0 || count == 0) {
		return NULL;
	}
	else if (id < 5000) {
		sprintf(g_tempBuffer, "%s #%d (%d)", g_weapons[id / 100].m_name, id % 100, count);
	}
	else if (id < 10000) {
		for (i = 22; i >= 0; i--) {
			if (g_equipment[i].m_id / 50 == id / 50) {
				sprintf(g_tempBuffer, "%s (%d)", g_equipment[i].m_name, count);
				break;
			}
		}
		if (i < 0) {
			sprintf(g_tempBuffer, "BAD CRITICAL %d", id);
		}
	}
	else {
		id -= 10000;
		sprintf(g_tempBuffer, "Ammo (%s #%d) #%d (%d)", g_weapons[id / 10000].m_name, id / 100 % 100, id % 100, count);
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, g_tempBuffer, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x1000a161
TextGlyph* DrawMore(ScreenField* p_tab)
{
	if (g_variant.m_unassigned[MECH_PTR_TO_S32(p_tab->m_data)].m_id <= 0) {
		return NULL;
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, "More...", p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x1000a1b0
TextGlyph* DrawYesNo(ScreenField* p_tab)
{
	MechS32 value;

	value = *(MechS32*) p_tab->m_data;
	return g_defaultFont
		->AddOverlayText(p_tab->m_left, p_tab->m_top, (MechChar*) (value ? "Yes" : "No"), p_tab->m_colors);
}

// Stack-slot permutation: id and text.
// FUNCTION: MW2SHELL 0x1000a208
TextGlyph* DrawEquipmentName(ScreenField* p_tab)
{
	MechS32 id;
	MechChar* text;

	id = MECH_PTR_TO_S32(p_tab->m_data);
	switch (id) {
	case 5000:
		text = "MASC";
		break;
	case 5401:
		text = "Right Lower Arm Actuator";
		break;
	case 5402:
		text = "Left Lower Arm Actuator";
		break;
	case 5451:
		text = "Right Hand Actuator";
		break;
	case 5452:
		text = "Left Hand Actuator";
		break;
	default:
		text = "";
	}

	return g_defaultFont->AddOverlayText(p_tab->m_left, p_tab->m_top, text, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x1000a2eb
void ShowComponent(ScreenField* p_tab)
{
	ScreenField* tabs;

	tabs = p_tab->m_arg;
	SetVideoFlags(10, 0x20, 0x20);
	SetVideoFlags(11, 0x20, 0x20);
	SetVideoFlags(12, 0x20, 0x20);
	SetVideoFlags(13, 0x20, 0x20);
	ShowVideo(14);
	HideFields(g_componentFields);
	g_componentFields = tabs;
	ShowFields(g_componentFields);
}

// FUNCTION: MW2SHELL 0x1000a36d
void ShowWeapons(ScreenField* p_tab)
{
	g_variant.m_selectedWeapon = -1;
	ShowComponent(p_tab);
	ShowVideo(11);
	ShowVideo(13);
	SetVideoFlags(14, 0x20, 0x20);
}

// FUNCTION: MW2SHELL 0x1000a3b5
void ShowArmor(ScreenField* p_tab)
{
	ShowComponent(p_tab);
	ShowVideo(10);
	ShowVideo(12);
	SetVideoFlags(14, 0x20, 0x20);
}

// FUNCTION: MW2SHELL 0x1000a3f3
void ShowCriticals(ScreenField* p_tab)
{
	ShowComponent(p_tab);
	ShowVideo(10);
	ShowVideo(11);
	ShowVideo(12);
	SetVideoFlags(14, 0x20, 0x20);
}

// FUNCTION: MW2SHELL 0x1000a43b
void EditVariantName(ScreenField* p_tab)
{
	if (p_tab->m_glyph != NULL) {
		delete p_tab->m_glyph;
	}

	EditTextField(
		g_defaultFont,
		p_tab->m_left,
		p_tab->m_top,
		(MechChar*) p_tab->m_data,
		p_tab->m_colors,
		0x1c,
		p_tab->m_width
	);
	p_tab->m_glyph = g_defaultFont->AddText(p_tab->m_left, p_tab->m_top, (MechChar*) p_tab->m_data, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x1000a4f0
void ClickFaster(ScreenField* p_tab)
{
	MechS32 rating;

	if (p_tab) {
	}

	rating = ((g_variant.m_walkingSpeed + 1) * (g_variant.m_maxMass / 100) * 5 + 4) / 5;
	if (rating < 10) {
		return;
	}
	if (rating > 400) {
		return;
	}

	rating = (rating - 10) / 5;
	if (g_variant.m_engine >= 10000) {
		g_variant.m_engine = rating + 10000;
	}
	else {
		g_variant.m_engine = rating;
	}

	UpdateEngine();
	UpdateHeatSinks();
	UpdateJumpJets();
	UpdateStats();
}

// The rating product differs only in evaluation order: the original divides m_maxMass before
// loading m_walkingSpeed - 1, as ClickFaster does; the order follows the unit's symbol table (it has
// flipped between the two functions as declarations moved).
// FUNCTION: MW2SHELL 0x1000a5a3
void ClickSlower(ScreenField* p_tab)
{
	MechS32 rating;

	if (p_tab) {
	}

	if (!g_variant.m_walkingSpeed) {
		return;
	}

	rating = ((g_variant.m_walkingSpeed - 1) * (g_variant.m_maxMass / 100) * 5 + 4) / 5;
	if (rating < 10) {
		return;
	}
	if (rating > 400) {
		return;
	}

	rating = (rating - 10) / 5;
	if (g_variant.m_engine >= 10000) {
		g_variant.m_engine = rating + 10000;
	}
	else {
		g_variant.m_engine = rating;
	}

	UpdateEngine();
	UpdateHeatSinks();
	UpdateJumpJets();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000a668
void ToggleEngineType(ScreenField* p_tab)
{
	MechS32 i;

	if (p_tab) {
	}

	if (g_variant.m_engine >= 10000) {
		g_variant.m_engine -= 10000;
		for (i = 0; i < 12; i++) {
			if (g_variant.m_criticals[1][i] == 5850) {
				g_variant.m_criticals[1][i] = 0;
			}
			if (g_variant.m_criticals[3][i] == 5850) {
				g_variant.m_criticals[3][i] = 0;
			}
		}
	}
	else {
		g_variant.m_engine += 10000;
		if (g_variant.m_criticals[3][0] > 0) {
			UnassignItem(g_variant.m_criticals[3][0]);
		}
		if (g_variant.m_criticals[3][1] > 0) {
			UnassignItem(g_variant.m_criticals[3][1]);
		}
		if (g_variant.m_criticals[1][0] > 0) {
			UnassignItem(g_variant.m_criticals[1][0]);
		}
		if (g_variant.m_criticals[1][1] > 0) {
			UnassignItem(g_variant.m_criticals[1][1]);
		}

		g_variant.m_criticals[3][0] = 5850;
		g_variant.m_criticals[3][1] = 5850;
		g_variant.m_criticals[1][0] = 5850;
		g_variant.m_criticals[1][1] = 5850;
	}

	UpdateEngine();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000a7ae
void AddJumpJet(ScreenField* p_tab)
{
	if (p_tab) {
	}

	if (g_variant.m_jumpJets >= g_variant.m_walkingSpeed) {
		return;
	}

	g_variant.m_jumpJets++;
	AddUnassigned(g_variant.m_jumpJets + 7000, 1);
	UpdateJumpJets();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000a803
void RemoveJumpJet(ScreenField* p_tab)
{
	if (p_tab) {
	}

	if (g_variant.m_jumpJets) {
		DeleteItem(g_variant.m_jumpJets + 7000);
		g_variant.m_jumpJets--;
	}

	UpdateJumpJets();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000a84d
void AddHeatSink(ScreenField* p_tab)
{
	if (p_tab) {
	}

	g_variant.m_heatSinkMass += 100;
	UpdateHeatSinks();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000a878
void RemoveHeatSink(ScreenField* p_tab)
{
	if (p_tab) {
	}

	if (g_variant.m_heatSinkMass) {
		g_variant.m_heatSinkMass -= 100;
	}

	UpdateHeatSinks();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000a8b0
void ToggleHeatSinkType(ScreenField* p_tab)
{
	MechS32 i;

	if (p_tab) {
	}

	g_variant.m_heatSinkType = 3 - g_variant.m_heatSinkType;
	UpdateHeatSinks();
	for (i = 1; i <= g_variant.m_externalHeatSinks; i++) {
		DeleteItem(i + 6000);
	}
	for (i = 1; i <= g_variant.m_externalHeatSinks; i++) {
		AddUnassigned(i + 6000, g_variant.m_heatSinkType);
	}

	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000a955
void ClickAddArmor(ScreenField* p_tab)
{
	if (p_tab) {
	}

	g_variant.m_armorMass += 50;
	UpdateArmor();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000a980
void ClickRemoveArmor(ScreenField* p_tab)
{
	if (p_tab) {
	}

	if (g_variant.m_armorMass) {
		g_variant.m_armorMass -= 50;
	}

	UpdateArmor();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000a9b8
void ToggleArmorType(ScreenField* p_tab)
{
	if (p_tab) {
	}

	g_variant.m_ferroFibrous = 1 - g_variant.m_ferroFibrous;
	if (!g_variant.m_ferroFibrous) {
		DeleteItem(9001);
		DeleteItem(9002);
		DeleteItem(9003);
		DeleteItem(9004);
		DeleteItem(9005);
		DeleteItem(9006);
		DeleteItem(9007);
	}
	else {
		AddUnassigned(9001, 1);
		AddUnassigned(9002, 1);
		AddUnassigned(9003, 1);
		AddUnassigned(9004, 1);
		AddUnassigned(9005, 1);
		AddUnassigned(9006, 1);
		AddUnassigned(9007, 1);
	}

	UpdateArmor();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000aac2
void ToggleInternalType(ScreenField* p_tab)
{
	if (p_tab) {
	}

	g_variant.m_endoSteel = 1 - g_variant.m_endoSteel;
	if (!g_variant.m_endoSteel) {
		DeleteItem(8001);
		DeleteItem(8002);
		DeleteItem(8003);
		DeleteItem(8004);
		DeleteItem(8005);
		DeleteItem(8006);
		DeleteItem(8007);
	}
	else {
		AddUnassigned(8001, 1);
		AddUnassigned(8002, 1);
		AddUnassigned(8003, 1);
		AddUnassigned(8004, 1);
		AddUnassigned(8005, 1);
		AddUnassigned(8006, 1);
		AddUnassigned(8007, 1);
	}

	UpdateArmor();
	UpdateStats();
	RedrawFields(g_componentFields);
	RedrawFields(g_screenFields);
}

// FUNCTION: MW2SHELL 0x1000abe8
void ClickAddWeapon(ScreenField* p_tab)
{
	MechS32 id;

	if (p_tab) {
	}

	if (g_variant.m_selectedWeapon < 0) {
		return;
	}

	id = AddWeapon(g_variant.m_selectedWeapon);
	if (id) {
		AddUnassigned(id, g_weapons[id / 100].m_criticals);
		if (CountAmmo(id) >= 10) {
			return;
		}

		if (g_weapons[id / 100].m_ammoPerTon) {
			id = AddAmmo(id * 100 + 10000);
			AddUnassigned(id, 1);
		}

		UpdateWeaponMass();
		UpdateAmmoMass();
		UpdateStats();
	}
}

// FUNCTION: MW2SHELL 0x1000acc4
void ClickDeleteWeapon(ScreenField* p_tab)
{
	if (p_tab) {
	}

	if (g_variant.m_selectedWeapon < 0) {
		return;
	}
	if (!(g_variant.m_selectedWeapon % 100)) {
		return;
	}

	RemoveWeapon(g_variant.m_selectedWeapon);
	DeleteItem(g_variant.m_selectedWeapon);
	g_variant.m_selectedWeapon = g_variant.m_weapons[0];
	UpdateWeaponMass();
	UpdateAmmoMass();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000ad3f
void ClickAddAmmo(ScreenField* p_tab)
{
	MechS32 id;

	if (p_tab) {
	}

	if (g_variant.m_selectedWeapon < 0) {
		return;
	}
	if (!(g_variant.m_selectedWeapon % 100)) {
		return;
	}
	if (CountAmmo(g_variant.m_selectedWeapon) >= 10) {
		return;
	}

	if (g_weapons[g_variant.m_selectedWeapon / 100].m_ammoPerTon) {
		id = AddAmmo(g_variant.m_selectedWeapon * 100 + 10000);
		AddUnassigned(id, 1);
		UpdateAmmoMass();
		UpdateStats();
	}
}

// FUNCTION: MW2SHELL 0x1000adf9
void ClickDeleteAmmo(ScreenField* p_tab)
{
	MechS32 count;

	if (p_tab) {
	}

	if (g_variant.m_selectedWeapon < 0) {
		return;
	}
	if (!(g_variant.m_selectedWeapon % 100)) {
		return;
	}

	count = CountAmmo(g_variant.m_selectedWeapon);
	if (!count) {
		return;
	}

	RemoveAmmo(g_variant.m_selectedWeapon * 100 + count + 10000);
	DeleteItem(g_variant.m_selectedWeapon * 100 + count + 10000);
	UpdateAmmoMass();
	UpdateStats();
}

// FUNCTION: MW2SHELL 0x1000aeaa
void SelectWeapon(ScreenField* p_tab)
{
	MechS32 id;

	id = *(MechS32*) p_tab->m_data;
	if (id < 0) {
		return;
	}

	if (g_variant.m_selectedWeapon == id && g_mouseState->GetDoubleClicked()) {
		ClickDeleteWeapon(p_tab);
	}

	g_variant.m_selectedWeapon = *(MechS32*) p_tab->m_data;
}

// FUNCTION: MW2SHELL 0x1000af16
void SelectWeaponType(ScreenField* p_tab)
{
	if (p_tab) {
	}

	if (MECH_PTR_TO_S32(p_tab->m_data) * 100 == g_variant.m_selectedWeapon) {
		if (g_mouseState->GetDoubleClicked()) {
			ClickAddWeapon(p_tab);
		}
	}
	else {
		g_variant.m_selectedWeapon = MECH_PTR_TO_S32(p_tab->m_data) * 100;
	}
}

// FUNCTION: MW2SHELL 0x1000af87
void NextLocation(ScreenField* p_tab)
{
	if (p_tab) {
	}

	g_locationSound->Start();
	g_variant.m_selectedLocation++;
	if (g_variant.m_selectedLocation >= 8) {
		g_variant.m_selectedLocation = 0;
	}
}

// FUNCTION: MW2SHELL 0x1000afc9
void SelectLocation(ScreenField* p_tab)
{
	g_locationSound->Start();
	g_variant.m_selectedLocation = MECH_PTR_TO_S32(p_tab->m_data);
}

// Stack-slot permutation: id and count.
// FUNCTION: MW2SHELL 0x1000afef
void ClickUnassigned(ScreenField* p_tab)
{
	MechS32 id;
	MechS32 count;

	id = g_variant.m_unassigned[MECH_PTR_TO_S32(p_tab->m_data)].m_id;
	count = g_variant.m_unassigned[MECH_PTR_TO_S32(p_tab->m_data)].m_criticals;
	if (id < 0) {
		return;
	}

	if (g_variant.m_selectedLocation >= 0) {
		if (id - id % 100 == 7000 && (g_variant.m_selectedLocation == 0 || g_variant.m_selectedLocation == 4 ||
									  g_variant.m_selectedLocation == 5)) {
			ShowDialog("Jump Jets may only|be assigned to torso|or leg sections.#Ok", 0);
			return;
		}

		if (!AssignItem(g_variant.m_selectedLocation, id, count)) {
			ShowDialog("Insufficient criticals|for item placement.#Ok", 0);
		}
	}
}

// FUNCTION: MW2SHELL 0x1000b0c2
void ClickMore(ScreenField* p_tab)
{
	if (p_tab) {
	}
}

// FUNCTION: MW2SHELL 0x1000b0dc
void ClickCritical(ScreenField* p_tab)
{
	MechS32 id;

	if (g_variant.m_selectedLocation < 0) {
		return;
	}

	id = g_variant.m_criticals[g_variant.m_selectedLocation][MECH_PTR_TO_S32(p_tab->m_data)];
	if (id < 0) {
		return;
	}

	if (id >= 5300 && id < 6000) {
		ShowDialog("Selected critical|can not be removed.#Ok", 0);
		return;
	}

	UnassignItem(id);
}

// Comparison operand order: the original loads m_armorAllocated into eax and compares m_armorFactor with
// it, the reverse of ours; flipping the source didn't change it.
// FUNCTION: MW2SHELL 0x1000b166
void AddFrontArmor(ScreenField* p_tab)
{
	MekVariant::Armor* armor;

	armor = &g_variant.m_armor[g_variant.m_selectedLocation];
	if (p_tab) {
	}

	if (g_variant.m_armorAllocated >= g_variant.m_armorFactor) {
		if (armor->m_rear > 0) {
			armor->m_front++;
			armor->m_rear--;
		}
	}
	else if (armor->m_rear < 0) {
		if (armor->m_maxArmor > armor->m_front) {
			armor->m_front++;
			g_variant.m_armorAllocated++;
		}
	}
	else if (armor->m_front + armor->m_rear < armor->m_maxArmor) {
		armor->m_front++;
		g_variant.m_armorAllocated++;
	}
	else if (armor->m_rear) {
		armor->m_front++;
		armor->m_rear--;
	}
}

// FUNCTION: MW2SHELL 0x1000b239
void RemoveFrontArmor(ScreenField* p_tab)
{
	MekVariant::Armor* armor;

	armor = &g_variant.m_armor[g_variant.m_selectedLocation];
	if (p_tab) {
	}

	if (armor->m_front > 0) {
		armor->m_front--;
		g_variant.m_armorAllocated--;
	}
}

// Comparison operand order: the original loads m_armorAllocated into eax and compares m_armorFactor with
// it, the reverse of ours; flipping the source didn't change it.
// FUNCTION: MW2SHELL 0x1000b284
void AddRearArmor(ScreenField* p_tab)
{
	MekVariant::Armor* armor;

	armor = &g_variant.m_armor[g_variant.m_selectedLocation];
	if (p_tab) {
	}

	if (armor->m_rear < 0) {
		return;
	}

	if (g_variant.m_armorAllocated >= g_variant.m_armorFactor) {
		if (armor->m_front > 0) {
			armor->m_front--;
			armor->m_rear++;
		}
	}
	else if (armor->m_front + armor->m_rear < armor->m_maxArmor) {
		armor->m_rear++;
		g_variant.m_armorAllocated++;
	}
	else if (armor->m_front) {
		armor->m_front--;
		armor->m_rear++;
	}
}

// FUNCTION: MW2SHELL 0x1000b339
void RemoveRearArmor(ScreenField* p_tab)
{
	MekVariant::Armor* armor;

	armor = &g_variant.m_armor[g_variant.m_selectedLocation];
	if (p_tab) {
	}

	if (armor->m_rear > 0) {
		armor->m_rear--;
		g_variant.m_armorAllocated--;
	}
}

// Toggles the flag at m_data and adds or removes the item id held in m_arg. The arm
// actuators (5401, 5402, 5451, 5452) take a fixed slot in the arms, replacing what is there.
// FUNCTION: MW2SHELL 0x1000b384
void ToggleEquipment(ScreenField* p_tab)
{
	MechS32* flag;
	MechS32 i;

	flag = (MechS32*) p_tab->m_data;
	if (*flag == 0) {
		*flag = 1;
	}
	else {
		*flag = 0;
	}

	if (*flag) {
		switch (MECH_PTR_TO_S32(p_tab->m_arg)) {
		case 5000:
			for (i = 1; i <= g_variant.m_mascCriticals; i++) {
				AddUnassigned(i + 5000, 1);
			}
			g_variant.m_equipmentMass = g_variant.m_mascCriticals * 100;
			UpdateStats();
			break;
		case 5401:
			if (g_variant.m_criticals[4][2] > 0) {
				UnassignItem(g_variant.m_criticals[4][2]);
			}
			g_variant.m_criticals[4][2] = 5401;
			break;
		case 5451:
			if (g_variant.m_criticals[4][3] > 0) {
				UnassignItem(g_variant.m_criticals[4][3]);
			}
			g_variant.m_criticals[4][3] = 5451;
			break;
		case 5402:
			if (g_variant.m_criticals[5][2] > 0) {
				UnassignItem(g_variant.m_criticals[5][2]);
			}
			g_variant.m_criticals[5][2] = 5402;
			break;
		case 5452:
			if (g_variant.m_criticals[5][3] > 0) {
				UnassignItem(g_variant.m_criticals[5][3]);
			}
			g_variant.m_criticals[5][3] = 5452;
			break;
		}
	}
	else {
		switch (MECH_PTR_TO_S32(p_tab->m_arg)) {
		case 5000:
			for (i = 1; i <= g_variant.m_mascCriticals; i++) {
				DeleteItem(i + 5000);
			}
			g_variant.m_equipmentMass = 0;
			UpdateStats();
			break;
		case 5401:
		case 5402:
		case 5451:
		case 5452:
			DeleteItem(MECH_PTR_TO_S32(p_tab->m_arg));
			break;
		}
	}
}

// Returns the first tab with a click callback that contains the point.
// Operand order: the original loads m_width before m_left; swapping them in the source didn't
// change it.
// FUNCTION: MW2SHELL 0x1000b5ed
ScreenField* FindFieldAt(ScreenField* p_tabs, MechS32 p_x, MechS32 p_y)
{
	if (p_tabs) {
		while (p_tabs->m_left != -1) {
			if (p_tabs->m_click != NULL && p_tabs->m_left <= p_x && p_x < p_tabs->m_left + p_tabs->m_width &&
				p_tabs->m_top <= p_y && p_y < p_tabs->m_top + p_tabs->m_height) {
				return p_tabs;
			}

			p_tabs++;
		}
	}

	return NULL;
}

// FUNCTION: MW2SHELL 0x1000b679
MechS32 FindMekItemInLocation(MechS32 p_id, MechS32 p_location, MechS32* p_slot)
{
	MechS32 i;

	for (i = 0; i < g_mekLocations[p_location].m_slotCount; i++) {
		if (g_mekLocations[p_location].m_items[i] == p_id) {
			if (p_slot != NULL) {
				*p_slot = i;
			}
			return TRUE;
		}
	}

	return FALSE;
}

// Stack-slot permutation: location and slot.
// FUNCTION: MW2SHELL 0x1000b6f4
MechS32 FindMekItem(MechS32 p_id, MechS32* p_location, MechS32* p_slot)
{
	MechS32 location;
	MechS32 slot;

	for (location = 0; location < 8; location++) {
		if (FindMekItemInLocation(p_id, location, &slot)) {
			if (p_location != NULL) {
				*p_location = location;
			}
			if (p_slot != NULL) {
				*p_slot = slot;
			}
			return TRUE;
		}
	}

	return FALSE;
}

// Saves the variant being edited to the .mek file p_name in the mek directory, creating the
// directory if needed. Returns 1, or 0 on failure.
// Not 100%: the original loads the index before the base in m_criticals[i][j] in the slot loop.
// FUNCTION: MW2SHELL 0x1000b771
MechS32 SaveMekFile(MechChar* p_name)
{
	MechS32 ammo;
	MechS32 i;
	MechS32 j;
	MechS32 k;
	FILE* file;

	memset(&g_mekHeader, 0, sizeof(g_mekHeader));
	memset(g_mekLocations, 0, sizeof(g_mekLocations));
	memset(g_mekWeapons, 0, sizeof(g_mekWeapons));
	memset(g_mekAmmo, 0, sizeof(g_mekAmmo));
	memset(g_mekVariantName, 0, sizeof(g_mekVariantName));

	for (i = 0; i < 10; i++) {
		if (g_variant.m_weapons[i] == -1) {
			break;
		}
		g_mekWeapons[i].m_id = g_variant.m_weapons[i];
		g_mekWeapons[i].m_weapon = -1;
	}
	g_mekHeader.m_weaponCount = i;

	for (i = 0; i < 25; i++) {
		if (g_variant.m_ammo[i] == -1) {
			break;
		}
		ammo = (g_variant.m_ammo[i] - 10000) / 10000;
		ammo = ammo * 100 + 10001;
		for (j = 0; j < 25; j++) {
			if (g_mekAmmo[j].m_id == 0) {
				break;
			}
			if (g_mekAmmo[j].m_id == ammo) {
				ammo++;
			}
		}
		g_mekAmmo[i].m_id = ammo;
		g_mekAmmo[i].m_weapon = (g_variant.m_ammo[i] - 10000) / 100;
	}
	g_mekHeader.m_ammoCount = i;

	for (i = 0; i < 8; i++) {
		g_mekLocations[i].m_unk0x26 = 1;
		g_mekLocations[i].m_slotCount = 12;
		for (j = 0; j < 12; j++) {
			if (g_variant.m_criticals[i][j] == -1) {
				g_mekLocations[i].m_slotCount = 6;
				g_mekLocations[i].m_items[j] = 0;
			}
			else if (g_variant.m_criticals[i][j] < 10000) {
				g_mekLocations[i].m_items[j] = g_variant.m_criticals[i][j];
			}
			else {
				for (k = 0; k < 25; k++) {
					if (g_variant.m_criticals[i][j] == g_variant.m_ammo[k]) {
						g_mekLocations[i].m_items[j] = g_mekAmmo[k].m_id;
						break;
					}
				}
			}
		}
	}

	g_mekLocations[0].m_internal = (MechS16) g_variant.m_armor[0].m_internal;
	g_mekLocations[1].m_internal = (MechS16) g_variant.m_armor[1].m_internal;
	g_mekLocations[2].m_internal = (MechS16) g_variant.m_armor[2].m_internal;
	g_mekLocations[3].m_internal = (MechS16) g_variant.m_armor[3].m_internal;
	g_mekLocations[4].m_internal = (MechS16) g_variant.m_armor[4].m_internal;
	g_mekLocations[5].m_internal = (MechS16) g_variant.m_armor[5].m_internal;
	g_mekLocations[6].m_internal = (MechS16) g_variant.m_armor[6].m_internal;
	g_mekLocations[7].m_internal = (MechS16) g_variant.m_armor[7].m_internal;
	g_mekLocations[0].m_front = (MechS16) g_variant.m_armor[0].m_front;
	g_mekLocations[1].m_front = (MechS16) g_variant.m_armor[1].m_front;
	g_mekLocations[2].m_front = (MechS16) g_variant.m_armor[2].m_front;
	g_mekLocations[3].m_front = (MechS16) g_variant.m_armor[3].m_front;
	g_mekLocations[4].m_front = (MechS16) g_variant.m_armor[4].m_front;
	g_mekLocations[5].m_front = (MechS16) g_variant.m_armor[5].m_front;
	g_mekLocations[6].m_front = (MechS16) g_variant.m_armor[6].m_front;
	g_mekLocations[7].m_front = (MechS16) g_variant.m_armor[7].m_front;
	g_mekLocations[1].m_rear = (MechS16) g_variant.m_armor[1].m_rear;
	g_mekLocations[2].m_rear = (MechS16) g_variant.m_armor[2].m_rear;
	g_mekLocations[3].m_rear = (MechS16) g_variant.m_armor[3].m_rear;

	g_mekHeader.m_tonnage = (g_variant.m_maxMass + 50) / 100;
	g_mekHeader.m_walkingSpeed = g_variant.m_walkingSpeed;
	g_mekHeader.m_jumpJets = g_variant.m_jumpJets;
	g_mekHeader.m_heatSinks = ((g_variant.m_heatSinkMass + 50) / 100 + 10) * g_variant.m_heatSinkType;
	strcpy(g_mekVariantName, g_variant.m_variantName);

	sprintf(g_mekPath, "mek\\%s", p_name);
	file = MechFopen(g_mekPath, "wb");
	if (file == NULL) {
		sprintf(g_mekPath, "mek\\");
		if (MechMakeDir(g_mekPath) != 0) {
			DebugPrint("Creating the mek directory failed\n");
			return 0;
		}

		sprintf(g_mekPath, "mek\\%s", p_name);
		file = MechFopen(g_mekPath, "wb");
		if (file == NULL) {
			return 0;
		}
	}

	fwrite(&g_mekHeader, 0x18, 1, file);
	fwrite(g_mekLocations, 0x28, 8, file);
	fwrite(g_mekWeapons, 8, g_mekHeader.m_weaponCount, file);
	fwrite(g_mekAmmo, 8, g_mekHeader.m_ammoCount, file);
	fwrite(g_mekVariantName, 0x32, 1, file);
	fclose(file);
	return 1;
}

// Returns the image of a .mek file: a user variant from the mek directory, or a standard one
// ("…std") from the project file.
// FUNCTION: MW2SHELL 0x1000bce5
void* LoadMekImage(MechChar* p_name)
{
	FILE* file;
	MechChar path[0x20];

	if (_strnicmp(p_name + 5, "std", 3)) {
		strcpy(path, "mek\\");
		strcat(path, p_name);
		if (strchr(p_name, '.') == NULL) {
			strcat(path, ".mek");
		}

		file = MechFopen(path, "rb");
		if (file == NULL) {
			return NULL;
		}

		fread(g_mekFileBuffer, 1, sizeof(g_mekFileBuffer), file);
		fclose(file);
		return g_mekFileBuffer;
	}

	return g_projectArchive->GetResourceByName(p_name, 6, "MEK");
}

// FUNCTION: MW2SHELL 0x1000be09
void ReleaseMekImage(MechChar* p_name)
{
	if (!_strnicmp(p_name + 5, "std", 3)) {
		g_projectArchive->ReleaseResourceByName(p_name, 6, "MEK");
	}
}

// Copies p_size bytes and returns the source position after them.
// FUNCTION: MW2SHELL 0x1000be4d
undefined* ReadBytes(undefined* p_dst, undefined* p_src, MechU32 p_size)
{
	memcpy(p_dst, p_src, p_size);
	return p_src + p_size;
}

// FUNCTION: MW2SHELL 0x1000be7a
void SetCritical(MechS32 p_location, MechS32 p_slot, MechS32 p_id)
{
	MechS32 count;
	MechS32 id;

	id = g_variant.m_criticals[p_location][p_slot];
	if (id) {
		count = RemoveCriticals(id);
	}

	g_variant.m_criticals[p_location][p_slot] = p_id;
	if (!PlaceCriticals(p_location, id, count)) {
		AddUnassigned(id, count);
	}
}

// Loads the .mek file p_name into the variant being edited, keeping the previous variant in
// g_previousVariant, and derives the engine, armor and internal structure from it.
// Not 100%: the stack slots of the locals are permuted, and m_items[j] of g_mekLocations loads the
// base before the index.
// FUNCTION: MW2SHELL 0x1000befe
void LoadMekFile(MechChar* p_name)
{
	MechS32 id;
	undefined* data;
	MechS32 i;
	MechS32 j;
	MechS32 k;

	data = (undefined*) LoadMekImage(p_name);
	if (data == NULL) {
		return;
	}

	g_previousVariant = g_variant;
	memset(&g_mekHeader, 0, sizeof(g_mekHeader));
	memset(g_mekLocations, 0, sizeof(g_mekLocations));
	memset(g_mekWeapons, 0, sizeof(g_mekWeapons));
	memset(g_mekAmmo, 0, sizeof(g_mekAmmo));
	memset(g_mekVariantName, 0, sizeof(g_mekVariantName));

	data = ReadBytes((undefined*) &g_mekHeader, data, 0x18);
	data = ReadBytes((undefined*) g_mekLocations, data, 0x140);
	ReadBytes((undefined*) g_mekWeapons, data, (g_mekHeader.m_weaponCount < 10 ? g_mekHeader.m_weaponCount : 10) * 8);
	data += g_mekHeader.m_weaponCount * 8;
	ReadBytes((undefined*) g_mekAmmo, data, (g_mekHeader.m_ammoCount < 25 ? g_mekHeader.m_ammoCount : 25) * 8);
	data += g_mekHeader.m_ammoCount * 8;
	data = ReadBytes((undefined*) g_mekVariantName, data, 0x32);
	ReleaseMekImage(p_name);

	memset(&g_variant, 0, sizeof(g_variant));
	for (i = 0; i < 78; i++) {
		g_variant.m_unassigned[i].m_id = -1;
	}

	for (i = 0; i < 10; i++) {
		if (i < g_mekHeader.m_weaponCount) {
			g_variant.m_weapons[i] = g_mekWeapons[i].m_id;
		}
		else {
			g_variant.m_weapons[i] = -1;
		}
	}

	for (i = 0; i < 25; i++) {
		g_variant.m_ammo[i] = -1;
	}
	for (i = 0; i < g_mekHeader.m_ammoCount; i++) {
		AddAmmo(g_mekAmmo[i].m_weapon * 100 + 10001);
		g_variant.m_ammoMass += 100;
	}

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 12; j++) {
			if (g_mekLocations[i].m_slotCount > j) {
				id = g_mekLocations[i].m_items[j];
				if (id >= 10000) {
					for (k = 0; k < 25; k++) {
						if (g_mekAmmo[k].m_id == id) {
							id = g_variant.m_ammo[k];
							break;
						}
					}
				}
				g_variant.m_criticals[i][j] = id;
			}
			else {
				g_variant.m_criticals[i][j] = -1;
			}
		}
	}

	strcpy(g_variant.m_variantName, g_mekVariantName);
	g_variant.m_maxMass = g_mekHeader.m_tonnage * 100;
	g_variant.m_walkingSpeed = g_mekHeader.m_walkingSpeed;
	g_variant.m_jumpJets = g_mekHeader.m_jumpJets;
	g_variant.m_heatSinkMass = (g_mekHeader.m_heatSinks - 10) * 100;
	g_variant.m_engineRating = (g_mekHeader.m_walkingSpeed * g_mekHeader.m_tonnage * 5 + 4) / 5;
	if (g_variant.m_engineRating < 10) {
		g_variant.m_engineRating = 10;
	}
	if (g_variant.m_engineRating > 400) {
		g_variant.m_engineRating = 400;
	}
	g_variant.m_engine = (g_variant.m_engineRating - 10) / 5;
	if (FindMekItemInLocation(5850, 1, NULL)) {
		g_variant.m_engine += 10000;
	}

	if (FindMekItem(8001, NULL, NULL)) {
		g_variant.m_endoSteel = 1;
	}
	if (FindMekItem(9001, NULL, NULL)) {
		g_variant.m_ferroFibrous = 1;
	}

	g_variant.m_armorFactor = 0;
	for (i = 0; i < 8; i++) {
		g_variant.m_armorFactor += g_mekLocations[i].m_rear + g_mekLocations[i].m_front;
	}
	if (g_variant.m_ferroFibrous == 1) {
		g_variant.m_armorMass = (MechS32) (g_variant.m_armorFactor / 19.2 * 100.0);
	}
	else {
		g_variant.m_armorMass = (MechS32) (g_variant.m_armorFactor / 16.0 * 100.0);
	}
	g_variant.m_armorMass = RoundTons(g_variant.m_armorMass * 2) / 2;
	g_variant.m_armorAllocated = g_variant.m_armorFactor;

	i = (g_variant.m_maxMass / 100 - 10) / 5;
	g_variant.m_armor[0].m_internal = 3;
	g_variant.m_armor[1].m_internal = g_internalStructure[i].m_sideTorso;
	g_variant.m_armor[2].m_internal = g_internalStructure[i].m_centerTorso;
	g_variant.m_armor[3].m_internal = g_internalStructure[i].m_sideTorso;
	g_variant.m_armor[4].m_internal = g_internalStructure[i].m_arm;
	g_variant.m_armor[5].m_internal = g_internalStructure[i].m_arm;
	g_variant.m_armor[6].m_internal = g_internalStructure[i].m_leg;
	g_variant.m_armor[7].m_internal = g_internalStructure[i].m_leg;
	g_variant.m_armor[0].m_maxArmor = 9;
	g_variant.m_armor[1].m_maxArmor = g_variant.m_armor[1].m_internal * 2;
	g_variant.m_armor[2].m_maxArmor = g_variant.m_armor[2].m_internal * 2;
	g_variant.m_armor[3].m_maxArmor = g_variant.m_armor[3].m_internal * 2;
	g_variant.m_armor[4].m_maxArmor = g_variant.m_armor[4].m_internal * 2;
	g_variant.m_armor[5].m_maxArmor = g_variant.m_armor[5].m_internal * 2;
	g_variant.m_armor[6].m_maxArmor = g_variant.m_armor[6].m_internal * 2;
	g_variant.m_armor[7].m_maxArmor = g_variant.m_armor[7].m_internal * 2;
	g_variant.m_armor[0].m_front = g_mekLocations[0].m_front;
	g_variant.m_armor[1].m_front = g_mekLocations[1].m_front;
	g_variant.m_armor[2].m_front = g_mekLocations[2].m_front;
	g_variant.m_armor[3].m_front = g_mekLocations[3].m_front;
	g_variant.m_armor[4].m_front = g_mekLocations[4].m_front;
	g_variant.m_armor[5].m_front = g_mekLocations[5].m_front;
	g_variant.m_armor[6].m_front = g_mekLocations[6].m_front;
	g_variant.m_armor[7].m_front = g_mekLocations[7].m_front;
	g_variant.m_armor[0].m_rear = -1;
	g_variant.m_armor[1].m_rear = g_mekLocations[1].m_rear;
	g_variant.m_armor[2].m_rear = g_mekLocations[2].m_rear;
	g_variant.m_armor[3].m_rear = g_mekLocations[3].m_rear;
	g_variant.m_armor[4].m_rear = -1;
	g_variant.m_armor[5].m_rear = -1;
	g_variant.m_armor[6].m_rear = -1;
	g_variant.m_armor[7].m_rear = -1;
	g_variant.m_selectedLocation = 0;

	g_variant.m_masc = 0;
	g_variant.m_rightLowerArm = 0;
	g_variant.m_rightHand = 0;
	g_variant.m_leftLowerArm = 0;
	g_variant.m_leftHand = 0;
	g_variant.m_mascCriticals = (g_variant.m_maxMass / 25 + 50) / 100;
	if (FindMekItem(5001, NULL, NULL)) {
		g_variant.m_equipmentMass = g_variant.m_mascCriticals * 100;
		g_variant.m_masc = 1;
	}

	RemoveCriticals(5301);
	RemoveCriticals(5351);
	g_variant.m_rightLowerArm = RemoveCriticals(5401);
	g_variant.m_rightHand = RemoveCriticals(5451);
	RemoveCriticals(5302);
	RemoveCriticals(5352);
	g_variant.m_leftLowerArm = RemoveCriticals(5402);
	g_variant.m_leftHand = RemoveCriticals(5452);
	SetCritical(4, 0, 5301);
	SetCritical(4, 1, 5351);
	if (g_variant.m_rightLowerArm) {
		SetCritical(4, 2, 5401);
	}
	if (g_variant.m_rightHand) {
		SetCritical(4, 3, 5451);
	}
	SetCritical(5, 0, 5302);
	SetCritical(5, 1, 5352);
	if (g_variant.m_leftLowerArm) {
		SetCritical(5, 2, 5402);
	}
	if (g_variant.m_leftHand) {
		SetCritical(5, 3, 5452);
	}

	UpdateEngine();
	UpdateJumpJets();
	UpdateWeaponMass();
	UpdateStats();

	g_variant.m_heatSinkType = 0;
	if (FindMekItem(6001, &i, &j)) {
		g_variant.m_criticals[i][j] = 0;
		if (FindMekItem(6001, NULL, NULL)) {
			g_variant.m_heatSinkType = 2;
		}
		else {
			g_variant.m_heatSinkType = 1;
		}
		g_variant.m_criticals[i][j] = 6001;
	}

	if (g_variant.m_heatSinkType == 0) {
		if (g_variant.m_usedMass > g_variant.m_maxMass && g_variant.m_heatSinkMass >= 1000 &&
			g_variant.m_heatSinkMass / 2 % 100 == 0) {
			g_variant.m_heatSinkType = 2;
		}
		else {
			g_variant.m_heatSinkType = 1;
		}
	}

	if (g_variant.m_heatSinkType == 2) {
		g_variant.m_heatSinkMass = g_variant.m_heatSinkMass / 2 - 500;
		UpdateStats();
	}

	g_variant.m_externalHeatSinks = -1;
	UpdateHeatSinks();
}

// Lists the variants of a mech: the standard ones from the project file in 1-99 (and the name
// of the first in 0), the user's from the mek directory in 100-199.
// FUNCTION: MW2SHELL 0x1000c8b8
void LoadMechBuildList(MechChar* p_prefix)
{
	MechS32 i;
	MechFileList* files;
	size_t count;
	size_t file;
	const char* name;

	memset(g_variantFiles, 0, sizeof(g_variantFiles));
	sprintf(g_variantFileName, "%s%02dstd", p_prefix, 0);
	strcpy(g_variantFiles[0], g_variantFileName);

	for (i = 1; i < 100; i++) {
		sprintf(g_variantFileName, "%s%02dstd", p_prefix, i);
		if (g_projectArchive->FindResourceId(g_variantFileName, 6) >= 0) {
			strcpy(g_variantFiles[i], g_variantFileName);
		}
	}

	sprintf(g_variantFileName, "mek\\%s??usr.mek", p_prefix);
	files = MechFindFiles(g_variantFileName);
	count = MechFileListCount(files);
	for (file = 0; file < count; file++) {
		name = MechFileListName(files, file);
		i = (name[3] - '0') * 10 + name[4] - '0' + 100;
		strncpy(g_variantFiles[i], name, 8);
		g_variantFiles[i][8] = '\0';
	}

	MechFileListFree(files);
}

// FUNCTION: MW2SHELL 0x1000ca74
void LoadChassis()
{
	sprintf(g_chassisVideoName, g_chassisVideoFormat, g_mechChassis[g_selectedChassis].m_code);
	SetVideoFlags(0x10, 0x40000000, 0x40000000);
	PlayVideo(0x10, g_chassisVideoName, g_chassisVideoLeft, g_chassisVideoTop, 0x88, 0xe);
	LoadMechBuildList(g_mechChassis[g_selectedChassis].m_prefix);
	LoadMekFile(g_variantFiles[g_selectedVariant]);
	g_variant.m_title[0] = '~';
	strcpy(&g_variant.m_title[1], g_mechChassis[g_selectedChassis].m_name);
}

// Plays the selected mech's name. Stack-slot permutation: audioData and audioSize.
// FUNCTION: MW2SHELL 0x1000cba7
void PlayChassisName()
{
	void* audioData;
	MechS32 audioSize;

	if (g_mechChassis[g_selectedChassis].m_nameSample < 0) {
		return;
	}

	g_mw2Database->GetDBItem(g_mechChassis[g_selectedChassis].m_nameSample, &audioData, &audioSize);
	if (g_chassisNameSound != NULL) {
		g_chassisNameSound->Stop();
		delete g_chassisNameSound;
	}

	g_chassisNameSound = new AudioSample(g_audioSubsystem, audioData, audioSize);
	g_chassisNameSound->SetVolume(40);
	g_chassisNameSound->Start();
}

// Comparison operand order: the original loads g_chassisCount into eax and compares
// g_selectedChassis with it, the reverse of ours; flipping the source didn't change it.
// FUNCTION: MW2SHELL 0x1000cce5
void NextChassis()
{
	g_selectedChassis++;
	if (g_selectedChassis >= g_chassisCount) {
		g_selectedChassis = 0;
	}

	g_selectedVariant = 0;
	PlayChassisName();
	LoadChassis();
}

// FUNCTION: MW2SHELL 0x1000cd2a
void PreviousChassis()
{
	g_selectedChassis--;
	if (g_selectedChassis < 0) {
		g_selectedChassis = g_chassisCount - 1;
	}

	g_selectedVariant = 0;
	PlayChassisName();
	LoadChassis();
}

// FUNCTION: MW2SHELL 0x1000cd65
void NextVariant()
{
	for (g_selectedVariant++; g_selectedVariant < 200; g_selectedVariant++) {
		if (g_variantFiles[g_selectedVariant][0]) {
			break;
		}
	}

	if (g_selectedVariant >= 200) {
		g_selectedVariant = 0;
	}

	LoadMekFile(g_variantFiles[g_selectedVariant]);
	g_variant.m_title[0] = '~';
	strcpy(&g_variant.m_title[1], g_mechChassis[g_selectedChassis].m_name);
	HideFields(g_componentFields);
	g_componentFields = NULL;
	RedrawFields(g_screenFields);
}

// FUNCTION: MW2SHELL 0x1000ce50
void PreviousVariant()
{
	if (g_selectedVariant == 0) {
		g_selectedVariant = 200;
	}

	for (g_selectedVariant--; g_selectedVariant >= 0; g_selectedVariant--) {
		if (g_variantFiles[g_selectedVariant][0]) {
			break;
		}
	}

	if (g_selectedVariant < 0) {
		g_selectedVariant = 0;
	}

	LoadMekFile(g_variantFiles[g_selectedVariant]);
	g_variant.m_title[0] = '~';
	strcpy(&g_variant.m_title[1], g_mechChassis[g_selectedChassis].m_name);
	HideFields(g_componentFields);
	g_componentFields = NULL;
	RedrawFields(g_screenFields);
}

// Saves the variant under the first free user slot. Returns FALSE when the variant is invalid.
// FUNCTION: MW2SHELL 0x1000cf4c
MechS32 SaveUserVariant()
{
	MechS32 i;

	if (g_variant.m_unassigned[0].m_id != -1) {
		ShowDialog("Invalid 'Mech specification:|Unassigned criticals detected.#Ok", 0);
		return FALSE;
	}

	if (g_variant.m_usedMass > g_variant.m_maxMass) {
		ShowDialog("Invalid 'Mech specification:|Chassis can not support|current mass.#Ok", 0);
		return FALSE;
	}

	for (i = 100; i < 200; i++) {
		if (!g_variantFiles[i][0]) {
			break;
		}
	}

	if (i >= 200) {
		ShowDialog("Error: Too many mechs|of this variant to save.#Ok", 0);
		return TRUE;
	}

	sprintf(g_userMekName, "%s%02dusr.mek", g_mechChassis[g_selectedChassis].m_prefix, i - 100);
	strcpy(g_variantFiles[i], g_userMekName);
	strncpy(g_variantFiles[i], g_userMekName, 8);
	g_variantFiles[i][8] = '\0';
	if (!SaveMekFile(g_userMekName)) {
		ShowDialog("Error saving 'Mech.#Ok", 0);
		return TRUE;
	}

	g_selectedVariant = i;
	return TRUE;
}

// The mech bay's field tables: the bay's own (g_mechBayFields), the customize screen's
// (g_customizeFields) and the ones its components switch to on the right.
// A field's m_top: a packed row and offset below the previous field (see ScreenField).
#define MB_ROW(row, offset) ((MechS32) (0x80000000 | ((row) << 4) | (offset)))
#define MB_TAB(left, top, width, height, colors, draw, click, data, next)                                              \
	{left, top, width, height, 0, colors, NULL, draw, click, (void*) (data), next}
#define MB_END {-1, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL}

extern ScreenField g_engineFields[];
extern ScreenField g_heatSinkFields[];
extern ScreenField g_jumpJetFields[];
extern ScreenField g_internalFields[];
extern ScreenField g_armorFields[];
extern ScreenField g_equipmentFields[];
extern ScreenField g_weaponFields[];
extern ScreenField g_criticalFields[];
extern ScreenField g_customizeFields[];
extern ScreenField g_mechBayFields[];

// GLOBAL: MW2SHELL 0x1005de80
ScreenField g_engineFields[] = {
	MB_TAB(444, 64, -1, -1, g_textColors, DrawLabel, NULL, "ENGINE", NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Rating", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawEngineRating, NULL, &g_variant.m_engine, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Type", NULL),
	MB_TAB(544, MB_ROW(0, 0), 50, -1, g_activeColors, DrawEngineType, ToggleEngineType, &g_variant.m_engine, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Manufactur", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawEngineMaker, NULL, &g_variant.m_engine, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Mass", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_engineMass, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Walking Speed", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawSpeed, NULL, &g_variant.m_walkingSpeed, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Running Speed", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawSpeed, NULL, &g_variant.m_runningSpeed, NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_activeColors, DrawLabel, ClickFaster, "FASTER", NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_activeColors, DrawLabel, ClickSlower, "SLOWER", NULL),
	MB_END,
};

// GLOBAL: MW2SHELL 0x1005e140
ScreenField g_heatSinkFields[] = {
	MB_TAB(444, 64, -1, -1, g_textColors, DrawLabel, NULL, "HEAT SINKS", NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Count", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawHeatSinkCount, NULL, &g_variant.m_heatSinkType, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Type", NULL),
	MB_TAB(
		544,
		MB_ROW(0, 0),
		50,
		-1,
		g_activeColors,
		DrawHeatSinkType,
		ToggleHeatSinkType,
		&g_variant.m_heatSinkType,
		NULL
	),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Mass", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_heatSinkMass, NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_activeColors, DrawLabel, AddHeatSink, "ADD", NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_activeColors, DrawLabel, RemoveHeatSink, "DELETE", NULL),
	MB_END,
};

// GLOBAL: MW2SHELL 0x1005e2f8
ScreenField g_jumpJetFields[] = {
	MB_TAB(444, 64, -1, -1, g_textColors, DrawLabel, NULL, "JUMP JETS", NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Count", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_jumpJets, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Mass", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_jumpJetMass, NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_activeColors, DrawLabel, AddJumpJet, "ADD", NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_activeColors, DrawLabel, RemoveJumpJet, "DELETE", NULL),
	MB_END,
};

// GLOBAL: MW2SHELL 0x1005e458
ScreenField g_internalFields[] = {
	MB_TAB(444, 64, -1, -1, g_textColors, DrawLabel, NULL, "INTERNAL STRUCTURE", NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Type", NULL),
	MB_TAB(
		544,
		MB_ROW(0, 0),
		50,
		-1,
		g_activeColors,
		DrawInternalType,
		ToggleInternalType,
		&g_variant.m_endoSteel,
		NULL
	),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Mass", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_internalMass, NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Head", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_armor[0], NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Right Torso", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_armor[1], NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Center Torso", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_armor[2], NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Left Torso", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_armor[3], NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Right Arm", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_armor[4], NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Left Arm", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_armor[5], NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Right Leg", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_armor[6], NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Left Leg", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_armor[7], NULL),
	MB_END,
};

// GLOBAL: MW2SHELL 0x1005e820
ScreenField g_armorFields[] = {
	MB_TAB(0, 0, -1, -1, g_textColors, DrawLocationMap, NULL, 0, NULL),
	MB_TAB(444, 64, -1, -1, g_textColors, DrawLabel, NULL, "ARMOR", NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Factor", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_armorFactor, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Allocated", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_armorAllocated, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Mass", NULL),
	MB_TAB(544, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_armorMass, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Type", NULL),
	MB_TAB(544, MB_ROW(0, 0), 100, -1, g_activeColors, DrawArmorType, ToggleArmorType, &g_variant.m_ferroFibrous, NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_activeColors, DrawLabel, ClickAddArmor, "ADD", NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_activeColors, DrawLabel, ClickRemoveArmor, "DELETE", NULL),
	MB_TAB(
		444,
		MB_ROW(3, 3),
		100,
		-1,
		g_activeColors,
		DrawSelectedLocation,
		NextLocation,
		&g_variant.m_selectedLocation,
		NULL
	),
	MB_TAB(554, MB_ROW(0, 0), -1, -1, g_textColors, DrawLocationArmor, NULL, 0, NULL),
	MB_TAB(584, MB_ROW(0, 0), -1, -1, g_textColors, DrawLocationArmor, NULL, 1, NULL),
	MB_TAB(551, 168, -1, -1, g_activeColors, DrawLabel, AddFrontArmor, "", NULL),
	MB_TAB(581, MB_ROW(0, 0), -1, -1, g_activeColors, DrawLabel, AddRearArmor, "", NULL),
	MB_TAB(551, 202, -1, -1, g_activeColors, DrawLabel, RemoveFrontArmor, "", NULL),
	MB_TAB(581, MB_ROW(0, 0), -1, -1, g_activeColors, DrawLabel, RemoveRearArmor, "", NULL),
	MB_TAB(224, 64, -1, -1, g_textColors, DrawLabel, NULL, "ARMOR ALLOCATION", NULL),
	MB_TAB(224, MB_ROW(2, 2), -1, -1, g_textColors, DrawLocationMaxArmor, NULL, 0, NULL),
	MB_TAB(344, MB_ROW(0, 0), -1, -1, g_textColors, DrawArmorAllocation, NULL, 0, NULL),
	MB_TAB(224, MB_ROW(1, 1), -1, -1, g_textColors, DrawLocationMaxArmor, NULL, 1, NULL),
	MB_TAB(344, MB_ROW(0, 0), -1, -1, g_textColors, DrawArmorAllocation, NULL, 1, NULL),
	MB_TAB(224, MB_ROW(1, 1), -1, -1, g_textColors, DrawLocationMaxArmor, NULL, 2, NULL),
	MB_TAB(344, MB_ROW(0, 0), -1, -1, g_textColors, DrawArmorAllocation, NULL, 2, NULL),
	MB_TAB(224, MB_ROW(1, 1), -1, -1, g_textColors, DrawLocationMaxArmor, NULL, 3, NULL),
	MB_TAB(344, MB_ROW(0, 0), -1, -1, g_textColors, DrawArmorAllocation, NULL, 3, NULL),
	MB_TAB(224, MB_ROW(1, 1), -1, -1, g_textColors, DrawLocationMaxArmor, NULL, 4, NULL),
	MB_TAB(344, MB_ROW(0, 0), -1, -1, g_textColors, DrawArmorAllocation, NULL, 4, NULL),
	MB_TAB(224, MB_ROW(1, 1), -1, -1, g_textColors, DrawLocationMaxArmor, NULL, 5, NULL),
	MB_TAB(344, MB_ROW(0, 0), -1, -1, g_textColors, DrawArmorAllocation, NULL, 5, NULL),
	MB_TAB(224, MB_ROW(1, 1), -1, -1, g_textColors, DrawLocationMaxArmor, NULL, 6, NULL),
	MB_TAB(344, MB_ROW(0, 0), -1, -1, g_textColors, DrawArmorAllocation, NULL, 6, NULL),
	MB_TAB(224, MB_ROW(1, 1), -1, -1, g_textColors, DrawLocationMaxArmor, NULL, 7, NULL),
	MB_TAB(344, MB_ROW(0, 0), -1, -1, g_textColors, DrawArmorAllocation, NULL, 7, NULL),
	MB_TAB(291, 223, 100, 34, g_textColors, NULL, SelectLocation, 0, NULL),
	MB_TAB(262, 258, 35, 100, g_textColors, NULL, SelectLocation, 1, NULL),
	MB_TAB(301, 258, 35, 100, g_textColors, NULL, SelectLocation, 2, NULL),
	MB_TAB(342, 258, 35, 100, g_textColors, NULL, SelectLocation, 3, NULL),
	MB_TAB(226, 256, 35, 100, g_textColors, NULL, SelectLocation, 4, NULL),
	MB_TAB(378, 256, 35, 100, g_textColors, NULL, SelectLocation, 5, NULL),
	MB_TAB(255, 362, 60, 100, g_textColors, NULL, SelectLocation, 6, NULL),
	MB_TAB(325, 362, 60, 100, g_textColors, NULL, SelectLocation, 7, NULL),
	MB_END,
};

// GLOBAL: MW2SHELL 0x1005efe0
ScreenField g_equipmentFields[] = {
	MB_TAB(444, 64, -1, -1, g_textColors, DrawLabel, NULL, "EQUIPMENT", NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Yes", NULL),
	MB_TAB(474, MB_ROW(0, 0), -1, -1, g_textColors, DrawLabel, NULL, "CASE", NULL),
	MB_TAB(
		444,
		MB_ROW(1, 1),
		100,
		-1,
		g_activeColors,
		DrawYesNo,
		ToggleEquipment,
		&g_variant.m_masc,
		(ScreenField*) 5000
	),
	MB_TAB(474, MB_ROW(0, 0), -1, -1, g_textColors, DrawEquipmentName, NULL, 5000, NULL),
	MB_TAB(
		444,
		MB_ROW(1, 1),
		100,
		-1,
		g_activeColors,
		DrawYesNo,
		ToggleEquipment,
		&g_variant.m_rightLowerArm,
		(ScreenField*) 5401
	),
	MB_TAB(474, MB_ROW(0, 0), -1, -1, g_textColors, DrawEquipmentName, NULL, 5401, NULL),
	MB_TAB(
		444,
		MB_ROW(1, 1),
		100,
		-1,
		g_activeColors,
		DrawYesNo,
		ToggleEquipment,
		&g_variant.m_rightHand,
		(ScreenField*) 5451
	),
	MB_TAB(474, MB_ROW(0, 0), -1, -1, g_textColors, DrawEquipmentName, NULL, 5451, NULL),
	MB_TAB(
		444,
		MB_ROW(1, 1),
		100,
		-1,
		g_activeColors,
		DrawYesNo,
		ToggleEquipment,
		&g_variant.m_leftLowerArm,
		(ScreenField*) 5402
	),
	MB_TAB(474, MB_ROW(0, 0), -1, -1, g_textColors, DrawEquipmentName, NULL, 5402, NULL),
	MB_TAB(
		444,
		MB_ROW(1, 1),
		100,
		-1,
		g_activeColors,
		DrawYesNo,
		ToggleEquipment,
		&g_variant.m_leftHand,
		(ScreenField*) 5452
	),
	MB_TAB(474, MB_ROW(0, 0), -1, -1, g_textColors, DrawEquipmentName, NULL, 5452, NULL),
	MB_END,
};

// GLOBAL: MW2SHELL 0x1005f248
ScreenField g_weaponFields[] = {
	MB_TAB(224, 64, -1, -1, g_textColors, DrawLabel, NULL, "WEAPONS AND AMMO", NULL),
	MB_TAB(224, MB_ROW(2, 2), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[0], NULL),
	MB_TAB(224, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[1], NULL),
	MB_TAB(224, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[2], NULL),
	MB_TAB(224, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[3], NULL),
	MB_TAB(224, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[4], NULL),
	MB_TAB(224, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[5], NULL),
	MB_TAB(224, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[6], NULL),
	MB_TAB(224, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[7], NULL),
	MB_TAB(224, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[8], NULL),
	MB_TAB(224, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[9], NULL),
	MB_TAB(224, MB_ROW(2, 2), -1, -1, g_activeColors, DrawLabel, ClickAddWeapon, "ADD WEAPON", NULL),
	MB_TAB(324, MB_ROW(0, 0), -1, -1, g_activeColors, DrawLabel, ClickAddAmmo, "ADD AMMO", NULL),
	MB_TAB(224, MB_ROW(1, 1), -1, -1, g_activeColors, DrawLabel, ClickDeleteWeapon, "DELETE WEAPON", NULL),
	MB_TAB(324, MB_ROW(0, 0), -1, -1, g_activeColors, DrawLabel, ClickDeleteAmmo, "DELETE AMMO", NULL),
	MB_TAB(224, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "WEAPON INFO", NULL),
	MB_TAB(224, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Type", NULL),
	MB_TAB(264, MB_ROW(0, 0), -1, -1, g_textColors, DrawWeaponInfo, NULL, 0, NULL),
	MB_TAB(224, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Heat", NULL),
	MB_TAB(274, MB_ROW(0, 0), -1, -1, g_textColors, DrawWeaponInfo, NULL, 1, NULL),
	MB_TAB(324, MB_ROW(0, 0), -1, -1, g_textColors, DrawLabel, NULL, "Mass", NULL),
	MB_TAB(374, MB_ROW(0, 0), -1, -1, g_textColors, DrawWeaponInfo, NULL, 7, NULL),
	MB_TAB(224, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Damage", NULL),
	MB_TAB(274, MB_ROW(0, 0), -1, -1, g_textColors, DrawWeaponInfo, NULL, 2, NULL),
	MB_TAB(324, MB_ROW(0, 0), -1, -1, g_textColors, DrawLabel, NULL, "Crit", NULL),
	MB_TAB(374, MB_ROW(0, 0), -1, -1, g_textColors, DrawWeaponInfo, NULL, 8, NULL),
	MB_TAB(224, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Range", NULL),
	MB_TAB(274, MB_ROW(0, 0), -1, -1, g_textColors, DrawWeaponInfo, NULL, 6, NULL),
	MB_TAB(324, MB_ROW(0, 0), -1, -1, g_textColors, DrawLabel, NULL, "Ammo", NULL),
	MB_TAB(374, MB_ROW(0, 0), -1, -1, g_textColors, DrawWeaponInfo, NULL, 9, NULL),
	MB_TAB(444, 64, -1, -1, g_textColors, DrawLabel, NULL, "WEAPONS TABLE", NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 22, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 23, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 24, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 21, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 25, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 26, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 27, NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 11, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 12, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 13, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 14, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 15, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 10, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 16, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 17, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 18, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 19, NULL),
	MB_TAB(444, MB_ROW(2, 2), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 6, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 5, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 4, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 9, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 8, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 7, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 3, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 2, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 1, NULL),
	MB_TAB(444, MB_ROW(1, 1), -1, -1, g_textColors, DrawWeaponTableEntry, SelectWeaponType, 0, NULL),
	MB_END,
};

// GLOBAL: MW2SHELL 0x1005fc70
ScreenField g_criticalFields[] = {
	MB_TAB(0, 0, -1, -1, g_textColors, DrawLocationMap, NULL, 0, NULL),
	MB_TAB(224, 64, 100, -1, g_activeColors, DrawLocationName, NextLocation, &g_variant.m_selectedLocation, NULL),
	MB_TAB(224, MB_ROW(1, 5), 100, -1, g_textColors, DrawCritical, ClickCritical, 0, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 1, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 2, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 3, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 4, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 5, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 6, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 7, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 8, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 9, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 10, NULL),
	MB_TAB(224, MB_ROW(1, 1), 100, -1, g_textColors, DrawCritical, ClickCritical, 11, NULL),
	MB_TAB(444, 64, -1, -1, g_textColors, DrawLabel, NULL, "UNASSIGNED CRITICALS", NULL),
	MB_TAB(444, MB_ROW(2, 2), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 0, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 1, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 2, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 3, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 4, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 5, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 6, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 7, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 8, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 9, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 10, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 11, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 12, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 13, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 14, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 15, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 16, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 17, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 18, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 19, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 20, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 21, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 22, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 23, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 24, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 25, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 26, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 27, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 28, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 29, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 30, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 31, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 32, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawUnassigned, ClickUnassigned, 33, NULL),
	MB_TAB(444, MB_ROW(1, 1), 100, -1, g_textColors, DrawMore, ClickMore, 34, NULL),
	MB_TAB(291, 223, 100, 34, g_textColors, NULL, SelectLocation, 0, NULL),
	MB_TAB(262, 258, 35, 100, g_textColors, NULL, SelectLocation, 1, NULL),
	MB_TAB(301, 258, 35, 100, g_textColors, NULL, SelectLocation, 2, NULL),
	MB_TAB(342, 258, 35, 100, g_textColors, NULL, SelectLocation, 3, NULL),
	MB_TAB(226, 256, 35, 100, g_textColors, NULL, SelectLocation, 4, NULL),
	MB_TAB(378, 256, 35, 100, g_textColors, NULL, SelectLocation, 5, NULL),
	MB_TAB(255, 362, 60, 100, g_textColors, NULL, SelectLocation, 6, NULL),
	MB_TAB(325, 362, 60, 100, g_textColors, NULL, SelectLocation, 7, NULL),
	MB_END,
};

// GLOBAL: MW2SHELL 0x10060698
ScreenField g_customizeFields[] = {
	MB_TAB(320, 4, -1, -1, g_textColors, DrawTitle, NULL, "~CUSTOMIZING", NULL),
	MB_TAB(320, 28, -1, -1, g_textColors, DrawTitle, NULL, g_variant.m_title, NULL),
	MB_TAB(20, 64, -1, -1, g_textColors, DrawLabel, NULL, "Variant:", NULL),
	MB_TAB(65, 64, 130, -1, g_activeColors, DrawLabel, EditVariantName, g_variant.m_variantName, NULL),
	MB_TAB(20, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "COMPONENT", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawLabel, NULL, "MASS", NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawLabel, NULL, "NOTES", NULL),
	MB_TAB(20, MB_ROW(2, 2), 100, -1, g_activeColors, DrawLabel, ShowComponent, "Engine", g_engineFields),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_engineMass, NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawEngineRating, NULL, &g_variant.m_engine, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Gyro", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_gyroMass, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Cockpit", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_cockpitMass, NULL),
	MB_TAB(20, MB_ROW(1, 1), 100, -1, g_activeColors, DrawLabel, ShowComponent, "Heat Sinks", g_heatSinkFields),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_heatSinkMass, NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawHeatSinkCount, NULL, &g_variant.m_heatSinkType, NULL),
	MB_TAB(20, MB_ROW(1, 1), 100, -1, g_activeColors, DrawLabel, ShowComponent, "Jump Jets", g_jumpJetFields),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_jumpJetMass, NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_jumpJets, NULL),
	MB_TAB(20, MB_ROW(1, 1), 100, -1, g_activeColors, DrawLabel, ShowComponent, "Internal", g_internalFields),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_internalMass, NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawInternalType, NULL, &g_variant.m_endoSteel, NULL),
	MB_TAB(20, MB_ROW(1, 1), 100, -1, g_activeColors, DrawLabel, ShowArmor, "Armor", g_armorFields),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_armorMass, NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawArmorType, NULL, &g_variant.m_ferroFibrous, NULL),
	MB_TAB(20, MB_ROW(1, 1), 100, -1, g_activeColors, DrawLabel, ShowWeapons, "Weapons", g_weaponFields),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_weaponMass, NULL),
	MB_TAB(20, MB_ROW(1, 1), 100, -1, g_activeColors, DrawLabel, ShowWeapons, "Ammo", g_weaponFields),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_ammoMass, NULL),
	MB_TAB(20, MB_ROW(1, 1), 100, -1, g_activeColors, DrawLabel, ShowComponent, "Equipment", g_equipmentFields),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_equipmentMass, NULL),
	MB_TAB(20, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Used Mass", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawUsedMass, NULL, &g_variant.m_usedMass, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Max Mass", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_maxMass, NULL),
	MB_TAB(20, MB_ROW(2, 2), -1, -1, g_activeColors, DrawLabel, ShowCriticals, "Assign Criticals", g_criticalFields),
	MB_END,
};

// GLOBAL: MW2SHELL 0x10060d20
ScreenField g_mechBayFields[] = {
	MB_TAB(320, 4, -1, -1, g_textColors, DrawTitle, NULL, g_callsignLine, NULL),
	MB_TAB(320, 28, -1, -1, g_textColors, DrawTitle, NULL, g_variant.m_title, NULL),
	MB_TAB(20, 64, -1, -1, g_textColors, DrawLabel, NULL, "Variant:", NULL),
	MB_TAB(65, 64, -1, -1, g_textColors, DrawLabel, NULL, g_variant.m_variantName, NULL),
	MB_TAB(20, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "COMPONENT", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawLabel, NULL, "MASS", NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawLabel, NULL, "NOTES", NULL),
	MB_TAB(20, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Engine", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_engineMass, NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawEngineRating, NULL, &g_variant.m_engine, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Gyro", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_gyroMass, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Cockpit", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_cockpitMass, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Heat Sinks", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_heatSinkMass, NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawHeatSinkCount, NULL, &g_variant.m_heatSinkType, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Jump Jets", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_jumpJetMass, NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawNumber, NULL, &g_variant.m_jumpJets, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Internal", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_internalMass, NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawInternalType, NULL, &g_variant.m_endoSteel, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Armor", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_armorMass, NULL),
	MB_TAB(147, MB_ROW(0, 0), -1, -1, g_textColors, DrawArmorType, NULL, &g_variant.m_ferroFibrous, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Weapons", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_weaponMass, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Ammo", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_ammoMass, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Equipment", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_equipmentMass, NULL),
	MB_TAB(20, MB_ROW(2, 2), -1, -1, g_textColors, DrawLabel, NULL, "Used Mass", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawUsedMass, NULL, &g_variant.m_usedMass, NULL),
	MB_TAB(20, MB_ROW(1, 1), -1, -1, g_textColors, DrawLabel, NULL, "Max Mass", NULL),
	MB_TAB(98, MB_ROW(0, 0), -1, -1, g_textColors, DrawMass, NULL, &g_variant.m_maxMass, NULL),
	MB_TAB(444, 64, -1, -1, g_textColors, DrawLabel, NULL, "WEAPONS AND AMMO", NULL),
	MB_TAB(444, MB_ROW(2, 2), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[0], NULL),
	MB_TAB(444, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[1], NULL),
	MB_TAB(444, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[2], NULL),
	MB_TAB(444, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[3], NULL),
	MB_TAB(444, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[4], NULL),
	MB_TAB(444, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[5], NULL),
	MB_TAB(444, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[6], NULL),
	MB_TAB(444, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[7], NULL),
	MB_TAB(444, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[8], NULL),
	MB_TAB(444, MB_ROW(1, 1), 130, -1, g_textColors, DrawWeaponEntry, SelectWeapon, &g_variant.m_weapons[9], NULL),
	MB_END,
};

#undef MB_ROW
#undef MB_TAB
#undef MB_END

// Opens the mech bay: its sounds, color maps, menu and videos, the chassis and variant of the
// current star's selected mech, and its fields.
// Not 100%: the stack slots of data, i and size are permuted.
// FUNCTION: MW2SHELL 0x1000d0d4
void DrawMechBay(TMPackDataBase* p_database, MechS32 p_campaign, size_t p_wParam)
{
	CustomStar* star;
	void* data;
	MechS32 i;
	MechChar* variant;
	MechS32 size;

	g_mechBayMessage = p_wParam;
	if (g_pickStarMech && p_wParam == c_msgStarConfig) {
		g_pickingStarMech = 1;
	}
	else {
		g_pickingStarMech = 0;
	}
	g_pickStarMech = 0;
	g_chassisNameSound = NULL;

	g_mw2Database->GetDBItem(0x50, &data, &size);
	g_locationSound = new AudioSample(g_audioSubsystem, data, size);
	g_locationSound->SetVolume(0x28);
	p_database->GetDBItem(0x4b, &data, &size);
	g_mechBayAmbience = new AudioSample(g_audioSubsystem, data, size);
	g_mechBayAmbience->EnableLoop();

	g_textColors[0] = 0xff;
	g_activeColors[0] = 0xff;
	g_warningColors[0] = 0xff;
	for (i = 1; i < 0x100; i++) {
		g_textColors[i] = g_activeColors[i] = g_warningColors[i] = i;
	}
	g_textColors[1] = 6;
	g_activeColors[1] = 1;
	g_warningColors[1] = 8;

	g_videoDriver->LoadBackground(p_database, g_mechBayScreens[p_campaign].m_picture);
	g_mechBayMenu = new ButtonMenu(
		g_videoDriver,
		g_defaultFont,
		FALSE,
		g_mechBayScreens[p_campaign].m_buttons,
		g_mechBayScreens[p_campaign].m_count
	);
	g_mechBayMenu->DisableButton(8);
	g_mechBayMenu->DisableButton(9);
	if (g_pickingStarMech) {
		g_mechBayMenu->DisableButton(0);
		g_mechBayMenu->DisableButton(6);
		g_mechBayMenu->DisableButton(7);
		g_mechBayMenu->DisableButton(10);
	}
	else {
		if (g_selectedChassis >= 15) {
			g_mechBayMenu->DisableButton(6);
		}
		if (g_selectedVariant < 100) {
			g_mechBayMenu->DisableButton(10);
		}
	}
	CloseAllVideos();

	switch (p_campaign) {
	case 0:
		PlayVideo(0, "awogrid", 0x8c, 0x136, 0x4a, 0);
		PlayVideo(1, "wwomp1", 0xc, 0x34, 0x42, 0);
		PlayVideo(10, "wwomp4ar", 0xd8, 0x34, 0x64, 0);
		PlayVideo(11, "wwomp7cr", 0x1b4, 0x34, 0x64, 0);
		PlayVideo(12, "wwomp4mp", 0x4b, 0xab, 0x21, 0);
		PlayVideo(13, "wwomp5wa", 0xd8, 0x34, 0x64, 0);
		PlayVideo(14, "wwomp1es", 0x1b4, 0x34, 0x42, 0);
		PlayVideo(4, "wwobkg", 0xdb, 0x1ad, 2, 0);
		PlayVideo(5, "wwostar", 0x197, 0x19a, 0x4a, 0);
		PlayVideo(6, "wwocn", 0x122, 0x1b1, 0x64, 0);
		PlayVideo(7, "wwocp", 0xf0, 0x1b3, 0x64, 0);
		PlayVideo(8, "wwovn", 0x12c, 0x1ad, 0x64, 0);
		PlayVideo(9, "wwovp", 0xdc, 0x1b0, 0x64, 0);
		g_chassisVideoFormat = g_wolfChassisVideo;
		g_chassisVideoLeft = 0x146;
		g_chassisVideoTop = 0x17f;
		g_warningColors[1] = 0x17;
		break;
	case 1:
		PlayVideo(0, "ajfgrid", 0x6c, 0x132, 0x4a, 0);
		PlayVideo(1, "wjfmp1", 0xc, 0x34, 0x42, 0);
		PlayVideo(10, "wjfmp4ar", 0xd8, 0x34, 0x64, 0);
		PlayVideo(11, "wjfmp7cr", 0x1b4, 0x34, 0x64, 0);
		PlayVideo(12, "wjfmp4mp", 0x4b, 0xab, 0x21, 0);
		PlayVideo(13, "wjfmp5wa", 0xd8, 0x34, 0x64, 0);
		PlayVideo(14, "wjfmp1es", 0x1b4, 0x34, 0x42, 0);
		PlayVideo(4, "wjfbkg", 0xd8, 0x1b2, 2, 0);
		PlayVideo(5, "wjfstar", 0x171, 0x1a0, 8, 0);
		PlayVideo(6, "wjfcn", 0x11e, 0x1b6, 0x64, 0);
		PlayVideo(7, "wjfcp", 0xf2, 0x1b6, 0x64, 0);
		PlayVideo(8, "wjfvn", 0x128, 0x1b2, 0x64, 0);
		PlayVideo(9, "wjfvp", 0xdc, 0x1b5, 0x64, 0);
		g_chassisVideoFormat = g_jadeFalconChassisVideo;
		g_chassisVideoLeft = 0x146;
		g_chassisVideoTop = 0x17f;
		g_warningColors[1] = 0x17;
		break;
	case 2:
		PlayVideo(0, "aiagrid", 0x6c, 0x132, 0x4a, 0);
		PlayVideo(1, "wiamp1", 0xc, 0x34, 0x42, 0);
		PlayVideo(10, "wiamp4ar", 0xd8, 0x34, 0x64, 0);
		PlayVideo(11, "wiamp7cr", 0x1b4, 0x34, 0x64, 0);
		PlayVideo(12, "wiamp4mp", 0x4b, 0xab, 0x21, 0);
		PlayVideo(13, "wiamp5wa", 0xd8, 0x34, 0x64, 0);
		PlayVideo(14, "wiamp1es", 0x1b4, 0x34, 0x42, 0);
		PlayVideo(4, "wiabkg1", 0xdb, 0x19e, 4, 0);
		PlayVideo(5, "wiastar", 0x19d, 0x1a1, 0x24, 0);
		PlayVideo(6, "wiacn", 0x109, 0x1ae, 0x24, 0);
		PlayVideo(7, "wiacp", 0xee, 0x1b5, 0x24, 0);
		PlayVideo(8, "wiavn", 0x12e, 0x1af, 0x24, 0);
		PlayVideo(9, "wiavp", 0xdf, 0x1b5, 0x24, 0);
		g_chassisVideoFormat = g_trialChassisVideo;
		g_chassisVideoLeft = 0x136;
		g_chassisVideoTop = 0x17f;
		g_warningColors[1] = 0xca;
		break;
	}

	if (!g_pickingStarMech) {
		SetStarMech(0, NULL, NULL);
	}

	if (p_campaign == 2) {
		g_chassisCount = GetChassisCount();
	}
	else {
		g_chassisCount = 15;
	}

	g_selectedVariant = 0;
	g_selectedChassis = GetStarMechChassis(-1);
	if (g_selectedChassis >= 0) {
		variant = GetStarMechVariant(-1);
		g_selectedVariant = (variant[3] - '0') * 10 + variant[4] - '0';
		if (_strnicmp(variant + 5, "std", 3)) {
			g_selectedVariant += 100;
		}
	}
	else {
		g_selectedChassis = 0;
	}
	if (g_selectedChassis >= g_chassisCount) {
		g_selectedChassis = 0;
	}

	g_mw2Database->GetDBItem(0x65, &data, &size);
	g_variantSound = new AudioSample(g_audioSubsystem, data, size);
	g_variantSound->SetVolume(0x32);
	g_mw2Database->GetDBItem(0x66, &data, &size);
	g_acceptSound = new AudioSample(g_audioSubsystem, data, size);
	g_acceptSound->SetVolume(0x32);

	star = GetStar(-1);
	sprintf(g_callsignLine, "~CALLSIGN: %s (%d.00 T MAX)", star->m_mechs[star->m_selected].m_pilot, star->m_tonnage);
	PlayChassisName();
	LoadChassis();
	g_screenFields = g_mechBayFields;
	ShowFields(g_screenFields);
	g_mechBayWParam = p_wParam;
	RegisterScreenFunction(MechBayCallback);
	UpdateVideos();
}

// MechBayCallback, the mech bay's frame, is implemented on the Rust side
// (src/shell/screens/mechlab.rs).
