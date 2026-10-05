#ifndef MECHBAY_H
#define MECHBAY_H

#include "audiosample.h"
#include "buttonmenu.h"
#include "decomp.h"
#include "mechchassis.h"
#include "screenfield.h"
#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

// An engine: its rating, its weight (in 1/100 t) and its maker. Ids from 10000 up are XL
// engines at half the weight.
// SIZE 0x0c
struct EngineType {
	MechS32 m_rating; // 0x00
	MechS32 m_weight; // 0x04
	MechChar* m_name; // 0x08
};

// A weapon: the stats the weapon info fields show. Masses are in 1/100 t. The table holds
// negative values in the fields up to 0x14, so those are signed.
// SIZE 0x28
struct Weapon {
	MechS32 m_heat;         // 0x00
	MechS32 m_damage;       // 0x04 — negative: per missile
	MechS32 m_minimumRange; // 0x08 — -1 for none
	MechS32 m_shortRange;   // 0x0c — -1 for none
	MechS32 m_mediumRange;  // 0x10 — -1 for none
	MechS32 m_range;        // 0x14 — -1 for none
	MechS32 m_mass;         // 0x18
	undefined4 m_criticals; // 0x1c — critical slots it takes
	MechS32 m_ammoPerTon;   // 0x20 — rounds per ton of ammo, 0 for weapons without ammo
	MechChar* m_name;       // 0x24
};

// SIZE 0x10
// The internal structure of a weight class (20 to 110 tons in steps of 5) per location.
struct InternalStructure {
	MechS32 m_centerTorso; // 0x00
	MechS32 m_sideTorso;   // 0x04
	MechS32 m_arm;         // 0x08
	MechS32 m_leg;         // 0x0c
};

// SIZE 0x08
// A piece of equipment other than a weapon: ids come in steps of 50 from 5000.
struct Equipment {
	MechS32 m_id;     // 0x00
	MechChar* m_name; // 0x04
};

// SIZE 0x28
// One location in the image of a .mek file: its armor and its critical slots.
struct MekLocation {
	MechS32 m_front;      // 0x00
	MechS32 m_rear;       // 0x04
	MechS32 m_internal;   // 0x08
	MechU16 m_items[12];  // 0x0c — item ids, ammunition as g_mekAmmo ids
	MechS16 m_slotCount;  // 0x24 — 12, or 6 for the head and legs
	undefined2 m_unk0x26; // 0x26 — always 1 in a saved .mek file, never read
};

// SIZE 0x18
// The header of a .mek file.
struct MekHeader {
	MechS32 m_tonnage;      // 0x00
	MechS32 m_walkingSpeed; // 0x04
	MechS32 m_jumpJets;     // 0x08
	MechS32 m_heatSinks;    // 0x0c — the count, doubled for double heat sinks
	MechS32 m_weaponCount;  // 0x10
	MechS32 m_ammoCount;    // 0x14
};

// SIZE 0x08
// A weapon or an ammunition entry of a .mek file.
struct MekItem {
	MechS32 m_id;     // 0x00 — the weapon id; for ammunition, 10000 + weapon type * 100 + n
	MechS32 m_weapon; // 0x04 — the weapon id the ammunition feeds, -1 for a weapon
};

// SIZE 0x7a8
// The variant being edited. DeleteItem copies an m_unassigned entry through the struct base
// (0x1005c640 + 0x4a0), which places the start; LoadMekFile copies the whole struct to
// g_previousVariant at 0x1005cde8 (0x7a8 bytes further), which bounds it.
// Masses are in 1/100 t. Items are ids: weapon w's #n is w * 100 + n (n from 1), equipment
// takes 5000-5999 in steps of 50 (5850 an XL engine's side torso slots), heat sink #n is
// 6000 + n, jump jet #n 7000 + n, Endo Steel and Ferro-Fibrous slots 8001-8007 and 9001-9007,
// and ammunition ton #n of weapon id w is 10000 + w * 100 + n.
struct MekVariant {
	// SIZE 0x08
	// An item waiting for critical slots.
	struct Item {
		MechS32 m_id;        // 0x00 — -1 when free
		MechS32 m_criticals; // 0x04 — slots it takes
	};

	// SIZE 0x10
	// One location's armor. The tab callbacks index the array through its base at 0x710.
	struct Armor {
		MechS32 m_internal; // 0x00 — internal structure
		MechS32 m_maxArmor; // 0x04 — twice the internal structure, 9 for the head
		MechS32 m_front;    // 0x08
		MechS32 m_rear;     // 0x0c — negative when the location has none
	};

	MechChar m_title[0x100];             // 0x00 — '~' and the mech's name
	MechChar m_variantName[0x40];        // 0x100
	undefined m_unk0x140[0x200 - 0x140]; // 0x140 — never accessed on its own
	MechS32 m_maxMass;                   // 0x200
	MechS32 m_usedMass;                  // 0x204
	MechS32 m_freeMass;                  // 0x208
	MechS32 m_engine;                    // 0x20c — index into g_engines, + 10000 for an XL engine
	MechS32 m_engineRating;              // 0x210
	MechS32 m_engineMass;                // 0x214
	MechS32 m_unk0x218;                  // 0x218 — 7 for an XL engine, else 0; never read
	MechS32 m_unk0x21c;                  // 0x21c — m_maxMass less m_engineMass; never read
	MechS32 m_gyroMass;                  // 0x220
	MechS32 m_cockpitMass;               // 0x224
	MechS32 m_heatSinkType;              // 0x228 — 1 single, 2 double
	MechS32 m_heatSinkMass;              // 0x22c — ten heat sinks come free
	MechS32 m_externalHeatSinks;         // 0x230 — heat sinks the engine can't hold, which take slots
	MechS32 m_jumpJetUnitMass;           // 0x234 — mass of one jump jet
	MechS32 m_jumpJetMass;               // 0x238
	undefined4 m_unk0x23c;               // 0x23c — never accessed on its own
	MechS32 m_endoSteel;                 // 0x240
	MechS32 m_internalMass;              // 0x244
	undefined4 m_unk0x248;               // 0x248 — never accessed on its own
	MechS32 m_ferroFibrous;              // 0x24c
	MechS32 m_armorMass;                 // 0x250
	undefined4 m_unk0x254;               // 0x254 — never accessed on its own
	MechS32 m_armorFactor;               // 0x258 — armor points m_armorMass buys
	MechS32 m_armorAllocated;            // 0x25c
	MechS32 m_weaponMass;                // 0x260
	undefined4 m_unk0x264;               // 0x264 — never accessed on its own
	MechS32 m_ammoMass;                  // 0x268
	undefined4 m_unk0x26c;               // 0x26c — never accessed on its own
	MechS32 m_equipmentMass;             // 0x270
	undefined4 m_unk0x274;               // 0x274 — never accessed on its own
	MechS32 m_walkingSpeed;              // 0x278 — shown as 10.8 kph per point
	MechS32 m_runningSpeed;              // 0x27c
	MechS32 m_jumpJets;                  // 0x280
	MechS32 m_ammo[25];                  // 0x284 — ammunition ids, -1 past the last
	MechS32 m_weapons[10];               // 0x2e8 — weapon ids, -1 past the last
	MechS32 m_selectedWeapon;            // 0x310 — a weapon id, or a weapon type * 100; -1 for none
	MechS32 m_selectedLocation;          // 0x314 — index into g_locationNames
	MechS32 m_criticals[8][12];          // 0x318 — item ids per location and slot, 0 when free
	undefined4 m_unk0x498;               // 0x498 — never accessed on its own
	MechS32 m_unassignedCount;           // 0x49c
	Item m_unassigned[78];               // 0x4a0
	Armor m_armor[8];                    // 0x710
	MechS32 m_masc;                      // 0x790
	MechS32 m_rightLowerArm;             // 0x794 — the actuator is fitted
	MechS32 m_rightHand;                 // 0x798
	MechS32 m_leftLowerArm;              // 0x79c
	MechS32 m_leftHand;                  // 0x7a0
	MechS32 m_mascCriticals;             // 0x7a4 — MASC's slots (items 5001 and up) and tons
};

// The functions and globals of mechbay.cpp that other units use.
extern MechChassis g_mechChassis[19];
extern MechS32 g_pickStarMech;
extern MekVariant g_variant;
extern ScreenField* g_componentFields;
extern ScreenField* g_screenFields;
extern AudioSample* g_locationSound;
extern MechS32 g_selectedChassis;
extern AudioSample* g_chassisNameSound;
extern ButtonMenu* g_mechBayMenu;
extern AudioSample* g_mechBayAmbience;
extern MechS32 g_mechBayWParam;
extern MechS32 g_unk0x1006178c;
extern MechS32 g_pickingStarMech;
extern AudioSample* g_acceptSound;
extern AudioSample* g_variantSound;
extern MechChar g_variantFiles[200][13];
extern MechS32 g_selectedVariant;
extern ScreenField g_engineFields[16];
extern ScreenField g_customizeFields[38];
extern ScreenField g_mechBayFields[48];
extern Equipment g_equipment[23];
extern MechChar* g_locationNames[8];
extern InternalStructure g_internalStructure[19];
extern MekVariant g_previousVariant;
extern EngineType g_engines[80];
extern Weapon g_weapons[31];
extern MechS32 g_armorTrimLocation;
extern MechS32 g_locationMapLefts[8];
extern MechS32 g_locationMapTops[8];
extern MechS32 g_chassisCount;
extern MechChar g_wolfChassisVideo[8];
extern MechChar g_jadeFalconChassisVideo[8];
extern MechChar g_trialChassisVideo[8];
extern MechChar* g_chassisVideoFormat;
extern undefined4 g_chassisVideoLeft;
extern undefined4 g_chassisVideoTop;
extern MechChar g_userMekName[0x20];
extern MechChar g_callsignLine[0x80];
extern MechChar g_tempBuffer[0x100];
extern MekItem g_mekAmmo[25];
extern MekHeader g_mekHeader;
extern MechChar g_chassisVideoName[0x10];
extern MechChar g_mekVariantName[0x32];
extern MekItem g_mekWeapons[10];
extern MechS32 g_mechBayMessage;
extern MechChar g_variantFileName[0x20];
extern undefined g_mekFileBuffer[0x800];
extern MekLocation g_mekLocations[8];
extern undefined g_warningColors[0x100];
extern undefined g_activeColors[0x100];
extern undefined g_textColors[0x100];
extern MechChar g_mekPath[0x20];
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
ScreenField* FindFieldAt(ScreenField* p_tabs, MechS32 p_x, MechS32 p_y);
void LoadChassis();
void NextChassis();
void PreviousChassis();
void NextVariant();
void PreviousVariant();
MechS32 SaveUserVariant();
void DrawMechBay(TMPackDataBase* p_database, MechS32 p_campaign, size_t p_wParam);
// The mech bay's frame, implemented on the Rust side (src/shell/screens/mechlab.rs)
extern "C" void MechBayCallback(
	TMPackDataBase* p_database,
	MechS32* p_campaign,
	MechU8* p_pilotChosen,
	char** p_scenario,
	MechS32 p_msg
);

#endif // MECHBAY_H
