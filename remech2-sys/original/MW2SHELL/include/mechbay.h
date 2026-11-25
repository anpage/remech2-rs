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
	int32_t m_rating;
	int32_t m_weight;
	char* m_name;
};

// A weapon: the stats the weapon info fields show. Masses are in 1/100 t. The table holds
// negative values in the fields up to 0x14, so those are signed.
struct Weapon {
	int32_t m_heat;
	int32_t m_damage;       // negative: per missile
	int32_t m_minimumRange; // -1 for none
	int32_t m_shortRange;   // -1 for none
	int32_t m_mediumRange;  // -1 for none
	int32_t m_range;        // -1 for none
	int32_t m_mass;
	uint32_t m_criticals; // critical slots it takes
	int32_t m_ammoPerTon; // rounds per ton of ammo, 0 for weapons without ammo
	char* m_name;
};

// The internal structure of a weight class (20 to 110 tons in steps of 5) per location.
struct InternalStructure {
	int32_t m_centerTorso;
	int32_t m_sideTorso;
	int32_t m_arm;
	int32_t m_leg;
};

// A piece of equipment other than a weapon: ids come in steps of 50 from 5000.
struct Equipment {
	int32_t m_id;
	char* m_name;
};

// One location in the image of a .mek file: its armor and its critical slots.
struct MekLocation {
	int32_t m_front;
	int32_t m_rear;
	int32_t m_internal;
	uint16_t m_items[12]; // item ids, ammunition as g_mekAmmo ids
	int16_t m_slotCount;  // 12, or 6 for the head and legs
	uint16_t m_flags;     // the simulator's MechSection::m_flags; the mech bay saves 1
};

// The header of a .mek file.
struct MekHeader {
	int32_t m_tonnage;
	int32_t m_walkingSpeed;
	int32_t m_jumpJets;
	int32_t m_heatSinks; // the count, doubled for double heat sinks
	int32_t m_weaponCount;
	int32_t m_ammoCount;
};

// A weapon or an ammunition entry of a .mek file.
struct MekItem {
	int32_t m_id;     // the weapon id; for ammunition, 10000 + weapon type * 100 + n
	int32_t m_weapon; // the weapon id the ammunition feeds, -1 for a weapon
};

// The variant being edited.
// Masses are in 1/100 t. Items are ids: weapon w's #n is w * 100 + n (n from 1), equipment
// takes 5000-5999 in steps of 50 (5850 an XL engine's side torso slots), heat sink #n is
// 6000 + n, jump jet #n 7000 + n, Endo Steel and Ferro-Fibrous slots 8001-8007 and 9001-9007,
// and ammunition ton #n of weapon id w is 10000 + w * 100 + n.
struct MekVariant {
	// An item waiting for critical slots.
	struct Item {
		int32_t m_id;        // -1 when free
		int32_t m_criticals; // slots it takes
	};

	// One location's armor (m_armor), which the tab callbacks index directly.
	struct Armor {
		int32_t m_internal; // internal structure
		int32_t m_maxArmor; // twice the internal structure, 9 for the head
		int32_t m_front;
		int32_t m_rear; // negative when the location has none
	};

	char m_title[0x100]; // '~' and the mech's name
	char m_variantName[0x40];
	int32_t m_maxMass;
	int32_t m_usedMass;
	int32_t m_freeMass;
	int32_t m_engine; // index into g_engines, + 10000 for an XL engine
	int32_t m_engineRating;
	int32_t m_engineMass;
	int32_t m_gyroMass;
	int32_t m_cockpitMass;
	int32_t m_heatSinkType;      // 1 single, 2 double
	int32_t m_heatSinkMass;      // ten heat sinks come free
	int32_t m_externalHeatSinks; // heat sinks the engine can't hold, which take slots
	int32_t m_jumpJetUnitMass;   // mass of one jump jet
	int32_t m_jumpJetMass;
	int32_t m_endoSteel;
	int32_t m_internalMass;
	int32_t m_ferroFibrous;
	int32_t m_armorMass;
	int32_t m_armorFactor; // armor points m_armorMass buys
	int32_t m_armorAllocated;
	int32_t m_weaponMass;
	int32_t m_ammoMass;
	int32_t m_equipmentMass;
	int32_t m_walkingSpeed; // shown as 10.8 kph per point
	int32_t m_runningSpeed;
	int32_t m_jumpJets;
	int32_t m_ammo[25];         // ammunition ids, -1 past the last
	int32_t m_weapons[10];      // weapon ids, -1 past the last
	int32_t m_selectedWeapon;   // a weapon id, or a weapon type * 100; -1 for none
	int32_t m_selectedLocation; // index into g_locationNames
	int32_t m_criticals[8][12]; // item ids per location and slot, 0 when free
	int32_t m_unassignedCount;
	Item m_unassigned[78];
	Armor m_armor[8];
	int32_t m_masc;
	int32_t m_rightLowerArm; // the actuator is fitted
	int32_t m_rightHand;
	int32_t m_leftLowerArm;
	int32_t m_leftHand;
	int32_t m_mascCriticals; // MASC's slots (items 5001 and up) and tons
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
