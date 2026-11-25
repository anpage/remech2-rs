#ifndef SHOTS_H
#define SHOTS_H

#include "careerrecord.h"
#include "effect.h"
#include "object.h"
#include "types.h"
#include "vector3.h"

// Shot::m_targetKind.
enum {
	c_shotTargetPlayer = 0x200,
	c_shotTargetGameThing = 0x400
};

// Shot::m_flags: set when a guided missile comes within 100 units of its target.
enum {
	c_shotProximityFuse = 0x8000
};

// Shot::m_impact, above the effect in the low byte: what the shot hit.
enum {
	c_impactMech = 0x100,
	c_impactThing = 0x200,
	c_impactGround = 0x400
};

// A projectile in flight.
typedef struct Shot {
	int32_t m_type;   // 7 for a shot that knocks mechs down
	int32_t m_impact; // the effect it sets off, with c_impactMech etc.
	int32_t m_velocity[3];
	int32_t m_steering[3];
	int32_t m_age;       // ticks since launch
	int32_t m_lifetime;  // ticks left
	int32_t m_swayPhase; // an angle (SwayShot)
	SceneObject* m_object;
	int32_t m_target;
	int32_t m_targetKind;
	uint32_t m_flags;
	int32_t m_shooter; // a player index
	int32_t m_damage;
	int32_t m_heat;    // added to the heat of the mech it hits
	int32_t m_tracked; // the tracked-shot view follows it
} Shot;

struct Player;
struct Shape;

// The functions and globals of shots.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_effectCameraActive;
	extern int32_t g_effectCameraEffect;
	extern int32_t g_lastLocalMissile;
	extern int32_t g_trackedShot;
	extern struct Player* g_launchEffectPlayer;
	extern int32_t g_effectCameraEnabled;
	extern int32_t g_lastHitShooter;
	extern int32_t g_nukeTimeLeft;
	extern int32_t g_nukeMaxRadius;
	extern int32_t g_trackedShotView[7];
	extern int32_t g_nukeRadius;
	extern Vector3 g_nukePosition;
	extern CareerRecord g_careerRecord;
	extern Shot g_shots[0xaf];
	extern Effect g_effects[0x100];
	extern int32_t g_savedLightX;
	extern int32_t g_savedLightY;
	extern int32_t g_savedLightZ;
	extern int32_t g_savedDirectionalLight;
	extern int32_t g_savedAmbientLight;
	extern int32_t g_savedDistanceFade;

	void FirstShots(void);
	void ResetEffectSlot(int32_t p_index);
	void ResetShotSlot(int32_t p_index);
	// Implemented on the Rust side (src/sim/shots.rs), around UpdateAllShotsC
	void UpdateAllShots(void);
	void UpdateAllShotsC(void);
	void UpdateShot(int32_t p_index);
	void SwayShot(Shot* p_shot, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	// Implemented on the Rust side (src/sim/shots.rs), around GuideMissileToTargetC
	void GuideMissileToTarget(Shot* p_shot, int32_t p_x, int32_t p_y, int32_t p_z);
	void GuideMissileToTargetC(Shot* p_shot, int32_t p_x, int32_t p_y, int32_t p_z);
	void DetonateShot(
		int32_t p_index,
		int32_t p_explode,
		int32_t p_x,
		int32_t p_y,
		int32_t p_z,
		int32_t p_camX,
		int32_t p_camY,
		int32_t p_camZ
	);
	void SpawnEffect(
		int32_t p_owner,
		int32_t p_type,
		int32_t p_x,
		int32_t p_y,
		int32_t p_z,
		int32_t p_camX,
		int32_t p_camY,
		int32_t p_camZ
	);
	void SpawnRotatedEffect(
		int32_t p_type,
		int32_t p_rotX,
		int32_t p_rotY,
		int32_t p_rotZ,
		int32_t p_x,
		int32_t p_y,
		int32_t p_z
	);
	void SpawnLaunchEffect(int32_t p_type, struct Player* p_player);
	void SpawnEffectEx(
		int32_t p_owner,
		int32_t p_type,
		int32_t p_x,
		int32_t p_y,
		int32_t p_z,
		int32_t p_camX,
		int32_t p_camY,
		int32_t p_camZ,
		int32_t p_rotX,
		int32_t p_rotY,
		int32_t p_rotZ
	);
	void UpdateEffects(void);
	void DamageMechsInRadius(int32_t p_owner, int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_radius, int32_t p_rate);
	void DamageThingsInRadius(int32_t p_owner, int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_radius, int32_t p_rate);
	int32_t* GetTrackedShotView(void);
	int32_t TrackLastShot(void);
	void KillGameThing(uint32_t p_index);
	void DamageGameThing(
		int32_t p_owner,
		struct Shape* p_shape,
		int32_t p_damage,
		int32_t p_x,
		int32_t p_y,
		int32_t p_z
	);
	void ScatterDebris(int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_count);
	void SaveCareerRecord(void);
	void HeatMechsNearFires(void);
	void StartNuke(struct Player* p_player);
	void UpdateNuke(void);

#ifdef __cplusplus
}
#endif

#endif // SHOTS_H
