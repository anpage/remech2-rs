#ifndef MECHCLASS_H
#define MECHCLASS_H

struct Mech;
struct Player;

#include "types.h"

// The functions and globals of mechclass.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_collisionSoundPlayed;
	extern int32_t g_infiniteJumpFuel;
	extern int32_t g_jettisonAmmoRequested;
	extern int32_t g_toggleMascRequested;
	extern int32_t g_mascEngaged;
	extern int32_t g_manualWeaponCycle;
	extern int32_t g_localMechLost;
	extern int32_t g_powerRequest;
	extern int32_t g_mechPoweredUp;
	extern int32_t g_localMechDestroyed;
	extern int32_t g_recenterLastHeading;
	extern int32_t g_lastMascRoll;
	extern int32_t g_ejectStarted;

	void FirstMech(struct Player* p_player);
	void UpdateMech(struct Mech* p_mech);
	// Implemented on the Rust side (src/sim/jumpjets.rs), around LateUpdateMechC
	void LateUpdateMech(struct Mech* p_mech);
	void LateUpdateMechC(struct Mech* p_mech);
	void UpdateLocalMech(struct Mech* p_mech);
	void DrawMechCockpit(struct Mech* p_mech);
	void ShutdownMech(struct Mech* p_mech);
	int32_t CreateMech(int32_t p_index, struct Player* p_player);
	void InitMechArrays(struct Mech* p_mech);
	int32_t GetMechAllocSize(void);
	int32_t GetLastSelectedWeapon(struct Player* p_player);
	void SetSelectedWeapon(struct Player* p_player, int32_t p_weapon);
	int32_t GetMechHeight(struct Player* p_player);

#ifdef __cplusplus
}
#endif

#endif // MECHCLASS_H
