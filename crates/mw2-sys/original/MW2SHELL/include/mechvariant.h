#ifndef MECHVARIANT_H
#define MECHVARIANT_H

#include "customstar.h"
#include "tmpackdatabase.h"
#include "types.h"

class AudioSample;
class ButtonMenu;
class TextGlyph;

// SIZE 0x08
// A formation: its label and its simulator option.
struct FormationOption {
	MechChar* m_label;  // 0x00
	MechChar* m_option; // 0x04
};

// SIZE 0x10
// A mech's place in a formation on the star screen: the star's mech it shows, where its video
// plays and which label position its name takes.
struct FormationSlot {
	MechS32 m_mech;  // 0x00
	MechS32 m_left;  // 0x04
	MechS32 m_top;   // 0x08
	MechS32 m_label; // 0x0c
};

// SIZE 0x30
// A formation's three positions.
struct FormationPositions {
	FormationSlot m_posts[3]; // 0x00
};

// The functions and globals of mechvariant.cpp that other units use.
extern FormationPositions g_formationLayouts[6];
extern FormationPositions g_jadeFalconFormationLayouts[6];
extern MechChar g_wolfStarVideo[8];
extern MechChar g_jadeFalconStarVideo[8];
extern MechChar g_trialStarVideo[8];
extern CustomStar g_playerStar;
extern CustomStar g_enemyStar;
extern CustomStar* g_selectedStar;
extern FormationOption g_formationOptions[6];
extern MechS32 g_starLabelLefts[4];
extern MechS32 g_starLabelTops[4];
extern MechS32 g_jadeFalconLabelLefts[4];
extern MechS32 g_jadeFalconLabelTops[3];
extern TextGlyph* g_formationLine;
extern TextGlyph* g_missionLine;
extern TextGlyph* g_starSizeLine;
extern TextGlyph* g_tonnageLine;
extern TextGlyph* g_starMassLine;
extern TextGlyph* g_positionGlyphs[3][3];
extern MechS32 g_mechMasses[3];
extern FormationPositions* g_formationLayout;
extern AudioSample* g_mechLabSound;
extern MechChar g_starInfoText[0x100];
extern MechS32* g_labelTops;
extern MechS32* g_labelLefts;
extern AudioSample* g_starSound;
extern ButtonMenu* g_starMenu;
extern MechChar g_fitTextBuffer[0x100];
extern MechChar* g_starVideoFormat;
void SaveStars();
void RestoreStars();
MechS32 SetStarMech(MechS32 p_index, MechChar* p_variant, MechChar* p_name);
MechChar* GetStarMechVariant(MechS32 p_index);
MechS32 GetStarMechChassis(MechS32 p_index);
MechS32 GetStarFormation(MechS32 p_star);
CustomStar* GetStar(MechS32 p_star);
void SelectStar(MechS32 p_star, MechS32 p_formation, MechS32 p_size, MechS32 p_count, MechS32 p_tonnage);
void WriteStarFiles();
void DrawStarConfig(TMPackDataBase* p_database, MechS32 p_campaign);

#endif // MECHVARIANT_H
