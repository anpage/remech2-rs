#ifndef MECHBAY_H
#define MECHBAY_H

#include "audiosample.h"
#include "buttonmenu.h"
#include "mechchassis.h"
#include "screenfield.h"
#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

// An engine: its rating, its weight (in 1/100 t) and its maker. Ids from 10000 up are XL
// engines at half the weight.
struct EngineType {
	int32_t m_rating; // 0x00
	int32_t m_weight; // 0x04
	char* m_name;     // 0x08
};

// A weapon: the stats the weapon info fields show. Masses are in 1/100 t. The table holds
// negative values in the fields up to 0x14, so those are signed.
struct Weapon {
	int32_t m_heat;         // 0x00
	int32_t m_damage;       // 0x04 — negative: per missile
	int32_t m_minimumRange; // 0x08 — -1 for none
	int32_t m_shortRange;   // 0x0c — -1 for none
	int32_t m_mediumRange;  // 0x10 — -1 for none
	int32_t m_range;        // 0x14 — -1 for none
	int32_t m_mass;         // 0x18
	uint32_t m_criticals;   // 0x1c — critical slots it takes
	int32_t m_ammoPerTon;   // 0x20 — rounds per ton of ammo, 0 for weapons without ammo
	char* m_name;           // 0x24
};

// The internal structure of a weight class (20 to 110 tons in steps of 5) per location.
struct InternalStructure {
	int32_t m_centerTorso; // 0x00
	int32_t m_sideTorso;   // 0x04
	int32_t m_arm;         // 0x08
	int32_t m_leg;         // 0x0c
};

// A piece of equipment other than a weapon: ids come in steps of 50 from 5000.
struct Equipment {
	int32_t m_id; // 0x00
	char* m_name; // 0x04
};

// One location in the image of a .mek file: its armor and its critical slots.
struct MekLocation {
	int32_t m_front;      // 0x00
	int32_t m_rear;       // 0x04
	int32_t m_internal;   // 0x08
	uint16_t m_items[12]; // 0x0c — item ids, ammunition as g_mekAmmo ids
	int16_t m_slotCount;  // 0x24 — 12, or 6 for the head and legs
	uint16_t m_unk0x26;   // 0x26 — always 1 in a saved .mek file, never read
};

// The header of a .mek file.
struct MekHeader {
	int32_t m_tonnage;      // 0x00
	int32_t m_walkingSpeed; // 0x04
	int32_t m_jumpJets;     // 0x08
	int32_t m_heatSinks;    // 0x0c — the count, doubled for double heat sinks
	int32_t m_weaponCount;  // 0x10
	int32_t m_ammoCount;    // 0x14
};

// A weapon or an ammunition entry of a .mek file.
struct MekItem {
	int32_t m_id;     // 0x00 — the weapon id; for ammunition, 10000 + weapon type * 100 + n
	int32_t m_weapon; // 0x04 — the weapon id the ammunition feeds, -1 for a weapon
};

// The variant being edited.
// Masses are in 1/100 t. Items are ids: weapon w's #n is w * 100 + n (n from 1), equipment
// takes 5000-5999 in steps of 50 (5850 an XL engine's side torso slots), heat sink #n is
// 6000 + n, jump jet #n 7000 + n, Endo Steel and Ferro-Fibrous slots 8001-8007 and 9001-9007,
// and ammunition ton #n of weapon id w is 10000 + w * 100 + n.
struct MekVariant {
	// An item waiting for critical slots.
	struct Item {
		int32_t m_id;        // 0x00 — -1 when free
		int32_t m_criticals; // 0x04 — slots it takes
	};

	// One location's armor. The tab callbacks index the array through its base at 0x710.
	struct Armor {
		int32_t m_internal; // 0x00 — internal structure
		int32_t m_maxArmor; // 0x04 — twice the internal structure, 9 for the head
		int32_t m_front;    // 0x08
		int32_t m_rear;     // 0x0c — negative when the location has none
	};

	char m_title[0x100];         // 0x00 — '~' and the mech's name
	char m_variantName[0x40];    // 0x100
	int32_t m_maxMass;           // 0x200
	int32_t m_usedMass;          // 0x204
	int32_t m_freeMass;          // 0x208
	int32_t m_engine;            // 0x20c — index into g_engines, + 10000 for an XL engine
	int32_t m_engineRating;      // 0x210
	int32_t m_engineMass;        // 0x214
	int32_t m_gyroMass;          // 0x220
	int32_t m_cockpitMass;       // 0x224
	int32_t m_heatSinkType;      // 0x228 — 1 single, 2 double
	int32_t m_heatSinkMass;      // 0x22c — ten heat sinks come free
	int32_t m_externalHeatSinks; // 0x230 — heat sinks the engine can't hold, which take slots
	int32_t m_jumpJetUnitMass;   // 0x234 — mass of one jump jet
	int32_t m_jumpJetMass;       // 0x238
	int32_t m_endoSteel;         // 0x240
	int32_t m_internalMass;      // 0x244
	int32_t m_ferroFibrous;      // 0x24c
	int32_t m_armorMass;         // 0x250
	int32_t m_armorFactor;       // 0x258 — armor points m_armorMass buys
	int32_t m_armorAllocated;    // 0x25c
	int32_t m_weaponMass;        // 0x260
	int32_t m_ammoMass;          // 0x268
	int32_t m_equipmentMass;     // 0x270
	int32_t m_walkingSpeed;      // 0x278 — shown as 10.8 kph per point
	int32_t m_runningSpeed;      // 0x27c
	int32_t m_jumpJets;          // 0x280
	int32_t m_ammo[25];          // 0x284 — ammunition ids, -1 past the last
	int32_t m_weapons[10];       // 0x2e8 — weapon ids, -1 past the last
	int32_t m_selectedWeapon;    // 0x310 — a weapon id, or a weapon type * 100; -1 for none
	int32_t m_selectedLocation;  // 0x314 — index into g_locationNames
	int32_t m_criticals[8][12];  // 0x318 — item ids per location and slot, 0 when free
	int32_t m_unassignedCount;   // 0x49c
	Item m_unassigned[78];       // 0x4a0
	Armor m_armor[8];            // 0x710
	int32_t m_masc;              // 0x790
	int32_t m_rightLowerArm;     // 0x794 — the actuator is fitted
	int32_t m_rightHand;         // 0x798
	int32_t m_leftLowerArm;      // 0x79c
	int32_t m_leftHand;          // 0x7a0
	int32_t m_mascCriticals;     // 0x7a4 — MASC's slots (items 5001 and up) and tons
};

// The functions and globals of mechbay.cpp that other units use.
extern MechChassis g_mechChassis[19];
extern int32_t g_pickStarMech;
extern MekVariant g_variant;
extern ScreenField* g_componentFields;
extern ScreenField* g_screenFields;
extern AudioSample* g_locationSound;
extern int32_t g_selectedChassis;
extern AudioSample* g_chassisNameSound;
extern ButtonMenu* g_mechBayMenu;
extern AudioSample* g_mechBayAmbience;
extern int32_t g_mechBayWParam;
extern int32_t g_pickingStarMech;
extern AudioSample* g_acceptSound;
extern AudioSample* g_variantSound;
extern char g_variantFiles[200][13];
extern int32_t g_selectedVariant;
extern ScreenField g_engineFields[16];
extern ScreenField g_customizeFields[38];
extern ScreenField g_mechBayFields[48];
extern Equipment g_equipment[23];
extern char* g_locationNames[8];
extern InternalStructure g_internalStructure[19];
extern MekVariant g_previousVariant;
extern EngineType g_engines[80];
extern Weapon g_weapons[31];
extern int32_t g_armorTrimLocation;
extern int32_t g_locationMapLefts[8];
extern int32_t g_locationMapTops[8];
extern int32_t g_chassisCount;
extern char g_wolfChassisVideo[8];
extern char g_jadeFalconChassisVideo[8];
extern char g_trialChassisVideo[8];
extern char* g_chassisVideoFormat;
extern uint32_t g_chassisVideoLeft;
extern uint32_t g_chassisVideoTop;
extern char g_userMekName[0x20];
extern char g_callsignLine[0x80];
extern char g_tempBuffer[0x100];
extern MekItem g_mekAmmo[25];
extern MekHeader g_mekHeader;
extern char g_chassisVideoName[0x10];
extern char g_mekVariantName[0x32];
extern MekItem g_mekWeapons[10];
extern int32_t g_mechBayMessage;
extern char g_variantFileName[0x20];
extern uint8_t g_mekFileBuffer[0x800];
extern MekLocation g_mekLocations[8];
extern uint8_t g_warningColors[0x100];
extern uint8_t g_activeColors[0x100];
extern uint8_t g_textColors[0x100];
extern char g_mekPath[0x20];
extern ScreenField g_heatSinkFields[10];
extern ScreenField g_jumpJetFields[8];
extern ScreenField g_internalFields[22];
extern ScreenField g_armorFields[45];
extern ScreenField g_equipmentFields[14];
extern ScreenField g_weaponFields[59];
extern ScreenField g_criticalFields[59];

void ShowFields(ScreenField* p_tabs);
void RedrawFields(ScreenField* p_tabs);
void HideFields(ScreenField* p_tabs);
ScreenField* FindFieldAt(ScreenField* p_tabs, int32_t p_x, int32_t p_y);
void LoadChassis();
void NextChassis();
void PreviousChassis();
void NextVariant();
void PreviousVariant();
int32_t SaveUserVariant();
void DrawMechBay(TMPackDataBase* p_database, int32_t p_campaign, size_t p_wParam);
// The mech bay's frame, implemented on the Rust side (src/shell/screens/mechlab.rs)
extern "C" void MechBayCallback(
	TMPackDataBase* p_database,
	int32_t* p_campaign,
	uint8_t* p_pilotChosen,
	char** p_scenario,
	int32_t p_msg
);

#endif // MECHBAY_H
