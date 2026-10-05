#ifndef MECHCLASS_H
#define MECHCLASS_H

struct Mech;
struct Player;

#include "decomp.h"
#include "types.h"

// The functions and globals of mechclass.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_collisionSoundPlayed;
	extern MechS32 g_infiniteJumpFuel;
	extern MechS32 g_jettisonAmmoRequested;
	extern MechS32 g_toggleMascRequested;
	extern MechS32 g_mascEngaged;
	extern MechS32 g_manualWeaponCycle;
	extern MechS32 g_localMechLost;
	extern MechS32 g_powerRequest;
	extern MechS32 g_mechPoweredUp;
	extern MechS32 g_localMechDestroyed;
	extern MechS32 g_unk0x100a2bf4;
	extern MechS32 g_unk0x100a2bfc;
	extern MechS32 g_unk0x100a2c00;
	extern MechS32 g_recenterLastHeading;
	extern MechS32 g_lastMascRoll;
	extern MechS32 g_ejectStarted;

	void FirstMech(struct Player* p_player);
	void UpdateMech(struct Mech* p_mech);
	// Implemented on the Rust side (src/sim/jumpjets.rs), around LateUpdateMechC
	void LateUpdateMech(struct Mech* p_mech);
	void LateUpdateMechC(struct Mech* p_mech);
	void UpdateLocalMech(struct Mech* p_mech);
	void DrawMechCockpit(struct Mech* p_mech);
	void ShutdownMech(struct Mech* p_mech);
	MechS32 CreateMech(MechS32 p_index, struct Player* p_player);
	void InitMechArrays(struct Mech* p_mech);
	MechS32 GetMechAllocSize(void);
	MechS32 GetLastSelectedWeapon(struct Player* p_player);
	void SetSelectedWeapon(struct Player* p_player, MechS32 p_weapon);
	MechS32 GetMechHeight(struct Player* p_player);

#ifdef __cplusplus
}
#endif

#endif // MECHCLASS_H
