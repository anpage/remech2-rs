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
	extern MechS32 g_remoteWeaponsFired[10];
	extern MechS32 g_localWeaponsFired[10];
	extern MechS32 g_singleWeaponFire;

	void ReleaseWeaponTriggers(struct Mech* p_mech);
	void UpdateWeaponFireState(struct Mech* p_mech);
	MechS32 SpawnShot(struct Player* p_player, struct WeaponSlot* p_slot);
	void SelectNextWeaponInGroup(struct Mech* p_mech, MechS32 p_wrap);
	void SelectNextWeapon(struct Mech* p_mech);
	void SelectNextWeaponGroup(struct Mech* p_mech);
	MechS32 SelectWeaponGroup(struct Mech* p_mech, MechS32 p_group);
	MechS32 IsSelectedWeaponReady(struct Mech* p_mech);
	MechS32 JettisonAmmo(struct Mech* p_mech);
	void LoadWeaponSounds(void);
	void SetWeaponGroup(struct Mech* p_mech, MechS32 p_index, MechS32 p_group);
	void SetSelectedWeaponGroup(MechS32 p_group);
	void CycleLocalWeaponGroup(void);
	void FireRemoteWeapons(struct Mech* p_mech);
	void FireWeaponGroup(void);
	void AddNextWeaponToGroup(struct Mech* p_mech);
	void UpdateWeaponLock(struct Mech* p_mech);
	struct Shape* UpdateAimDistance(struct Player* p_player);
	MechS32 GetAimRange(struct Player* p_player);
	void GetMechAimDirection(struct Player* p_player, MechS32* p_x, MechS32* p_y, MechS32* p_z);
	void BuildAimRay(struct Player* p_player, struct Ray* p_ray);
	void GetEyeAimDirection(struct Player* p_player, MechS32* p_x, MechS32* p_y, MechS32* p_z);
	void GetFiringPosition(struct Player* p_player, MechS32* p_x, MechS32* p_y, MechS32* p_z);
	void PlaceAtFiringObj(struct Player* p_player, struct SceneObject* p_obj);
	void SpawnLaunchFx(
		struct Player* p_player,
		struct SceneObject* p_obj,
		MechS32 p_dx,
		MechS32 p_dy,
		MechS32 p_dz,
		MechS32 p_spread
	);

#ifdef __cplusplus
}
#endif

#endif // WEAPONS_H
