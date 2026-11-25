#ifndef WEAPONDEF_H
#define WEAPONDEF_H

#include "types.h"

// A weapon type, from the table of weapon definitions (g_weaponDefs: LLASER, MLASER, ...).
typedef struct WeaponDef {
	int32_t m_shotType;      // the Shot::m_type a launch takes from the free shots
	int32_t m_impact;        // the effect the shot sets off (Shot::m_impact)
	int32_t m_launchEffect;  // an effect started at each launch (SpawnLaunchEffect)
	int32_t m_launchEffect2; // another
	int32_t m_autoRepeat;    // the next volley follows at once while the trigger is held
	int32_t m_launchesShot;  // a launch takes a shot
	int32_t m_guided;        // the shots follow the locked target
	int32_t m_volley;        // shots per volley
	int32_t m_volleysPerBin; // volleys per ammunition bin, or -1 for unlimited ammunition
	int32_t m_sound;         // the launch sound, or <= 0
	int32_t m_gravity;       // scales the gravity on the shot
	int32_t m_speed;
	int32_t m_damage;
	int32_t m_heat;     // added to the firing mech's heat per shot
	int32_t m_shotHeat; // added to the heat of the mech the shot hits
	int32_t m_shortRange;
	int32_t m_longRange; // a target lock needs the target between the two ranges
	int32_t m_recycle;   // ticks between volleys
	int32_t m_interval;  // ticks between the shots of a volley
	int32_t m_lifetime;  // the shot's, in ticks
	char m_name[8];
} WeaponDef;

#endif // WEAPONDEF_H
