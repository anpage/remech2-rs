#ifndef MECHVARIANT_H
#define MECHVARIANT_H

#include "customstar.h"
#include "tmpackdatabase.h"
#include "types.h"

class AudioSample;
class ButtonMenu;
class TextGlyph;

// A formation: its label and its simulator option.
struct FormationOption {
	char* m_label;  // 0x00
	char* m_option; // 0x04
};

// A mech's place in a formation on the star screen: the star's mech it shows, where its video
// plays and which label position its name takes.
struct FormationSlot {
	int32_t m_mech;  // 0x00
	int32_t m_left;  // 0x04
	int32_t m_top;   // 0x08
	int32_t m_label; // 0x0c
};

// A formation's three positions.
struct FormationPositions {
	FormationSlot m_posts[3]; // 0x00
};

// The functions and globals of mechvariant.cpp that other units use.
extern FormationPositions g_formationLayouts[6];
extern FormationPositions g_jadeFalconFormationLayouts[6];
extern char g_wolfStarVideo[8];
extern char g_jadeFalconStarVideo[8];
extern char g_trialStarVideo[8];
extern CustomStar g_playerStar;
extern CustomStar g_enemyStar;
extern CustomStar* g_selectedStar;
extern FormationOption g_formationOptions[6];
extern int32_t g_starLabelLefts[4];
extern int32_t g_starLabelTops[4];
extern int32_t g_jadeFalconLabelLefts[4];
extern int32_t g_jadeFalconLabelTops[3];
extern TextGlyph* g_formationLine;
extern TextGlyph* g_missionLine;
extern TextGlyph* g_starSizeLine;
extern TextGlyph* g_tonnageLine;
extern TextGlyph* g_starMassLine;
extern TextGlyph* g_positionGlyphs[3][3];
extern int32_t g_mechMasses[3];
extern FormationPositions* g_formationLayout;
extern AudioSample* g_mechLabSound;
extern char g_starInfoText[0x100];
extern int32_t* g_labelTops;
extern int32_t* g_labelLefts;
extern AudioSample* g_starSound;
extern ButtonMenu* g_starMenu;
extern char g_fitTextBuffer[0x100];
extern char* g_starVideoFormat;
void SaveStars();
void RestoreStars();
int32_t SetStarMech(int32_t p_index, char* p_variant, char* p_name);
char* GetStarMechVariant(int32_t p_index);
int32_t GetStarMechChassis(int32_t p_index);
int32_t GetStarFormation(int32_t p_star);
CustomStar* GetStar(int32_t p_star);
void SelectStar(int32_t p_star, int32_t p_formation, int32_t p_size, int32_t p_count, int32_t p_tonnage);
void WriteStarFiles();
void DrawStarConfig(TMPackDataBase* p_database, int32_t p_campaign);

#endif // MECHVARIANT_H
