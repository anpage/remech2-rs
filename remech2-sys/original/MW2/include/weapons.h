#ifndef WEAPONS_H
#define WEAPONS_H

#include "decomp.h"
#include "types.h"
#include "weapondef.h"

struct SceneObject;
struct Mech;
struct Player;
struct Ray;
struct Shape;
struct WeaponSlot;

// The functions and globals of weapons.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern struct Shape* g_aimedShape;
	extern int32_t g_remoteWeaponsFired[10];
	extern int32_t g_localWeaponsFired[10];
	extern int32_t g_singleWeaponFire;

	void ReleaseWeaponTriggers(struct Mech* p_mech);
	void UpdateWeaponFireState(struct Mech* p_mech);
	int32_t SpawnShot(struct Player* p_player, struct WeaponSlot* p_slot);
	void SelectNextWeaponInGroup(struct Mech* p_mech, int32_t p_wrap);
	void SelectNextWeapon(struct Mech* p_mech);
	void SelectNextWeaponGroup(struct Mech* p_mech);
	int32_t SelectWeaponGroup(struct Mech* p_mech, int32_t p_group);
	int32_t IsSelectedWeaponReady(struct Mech* p_mech);
	int32_t JettisonAmmo(struct Mech* p_mech);
	void LoadWeaponSounds(void);
	void SetWeaponGroup(struct Mech* p_mech, int32_t p_index, int32_t p_group);
	void SetSelectedWeaponGroup(int32_t p_group);
	void CycleLocalWeaponGroup(void);
	void FireRemoteWeapons(struct Mech* p_mech);
	void FireWeaponGroup(void);
	void AddNextWeaponToGroup(struct Mech* p_mech);
	void UpdateWeaponLock(struct Mech* p_mech);
	struct Shape* UpdateAimDistance(struct Player* p_player);
	int32_t GetAimRange(struct Player* p_player);
	void GetMechAimDirection(struct Player* p_player, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	void BuildAimRay(struct Player* p_player, struct Ray* p_ray);
	void GetEyeAimDirection(struct Player* p_player, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	void GetFiringPosition(struct Player* p_player, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	void PlaceAtFiringObj(struct Player* p_player, struct SceneObject* p_obj);
	void SpawnLaunchFx(
		struct Player* p_player,
		struct SceneObject* p_obj,
		int32_t p_dx,
		int32_t p_dy,
		int32_t p_dz,
		int32_t p_spread
	);

#ifdef __cplusplus
}
#endif

#endif // WEAPONS_H
