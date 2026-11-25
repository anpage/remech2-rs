#ifndef WEAPONDEF_H
#define WEAPONDEF_H

#include "types.h"

// A weapon type, from the table of weapon definitions (g_weaponDefs: LLASER, MLASER, ...).
typedef struct WeaponDef {
	int32_t m_shotType;      // 0x00 — the Shot::m_type a launch takes from the free shots
	int32_t m_impact;        // 0x04 — the effect the shot sets off (Shot::m_impact)
	int32_t m_launchEffect;  // 0x08 — an effect started at each launch (SpawnLaunchEffect)
	int32_t m_launchEffect2; // 0x0c — another
	int32_t m_autoRepeat;    // 0x10 — the next volley follows at once while the trigger is held
	int32_t m_launchesShot;  // 0x14 — a launch takes a shot
	int32_t m_guided;        // 0x18 — the shots follow the locked target
	int32_t m_volley;        // 0x1c — shots per volley
	int32_t m_volleysPerBin; // 0x20 — volleys per ammunition bin, or -1 for unlimited ammunition
	int32_t m_sound;         // 0x24 — the launch sound, or <= 0
	int32_t m_gravity;       // 0x28 — scales the gravity on the shot
	int32_t m_speed;         // 0x2c
	int32_t m_damage;        // 0x30
	int32_t m_heat;          // 0x34 — added to the firing mech's heat per shot
	int32_t m_shotHeat;      // 0x38 — added to the heat of the mech the shot hits
	int32_t m_shortRange;    // 0x3c
	int32_t m_longRange;     // 0x40 — a target lock needs the target between the two ranges
	int32_t m_recycle;       // 0x44 — ticks between volleys
	int32_t m_interval;      // 0x48 — ticks between the shots of a volley
	int32_t m_lifetime;      // 0x4c — the shot's, in ticks
	char m_name[8];          // 0x50
} WeaponDef;

#endif // WEAPONDEF_H
