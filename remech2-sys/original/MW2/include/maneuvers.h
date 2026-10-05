#ifndef MANEUVERS_H
#define MANEUVERS_H

#include "maneuvertable.h"
#include "point.h"
#include "types.h"

struct Ray;

struct Mech;
struct Player;
struct Shape;
struct WeaponSlot;

// The maneuvers an AI player makes in the attack state (Player::m_maneuver), by the names the AI's
// log gives them (g_aiBehaviorNames).
enum {
	c_maneuverStupid = 0,  // walk at the target
	c_maneuverBehind = 1,  // take a place around the goal player (ChooseFlankPlace)
	c_maneuverAchick = 2,  // close in
	c_maneuverAsrp = 3,    // close in, weaving
	c_maneuverAjmpin = 4,  // jump until above the target
	c_maneuverAdfa = 5,    // jump at the target: death from above
	c_maneuverKama = 6,    // run at the target and self-destruct
	c_maneuverWchick = 7,  // move to a point to its side, facing the goal
	c_maneuverWbackp = 8,  // back off
	c_maneuverWpeek = 9,   // jump up, fire and come down
	c_maneuverAvoid = 10,  // back away from what it is stuck on
	c_maneuverSprint = 11, // jump aside, dodging a shot (DodgeShot)
	c_maneuverCircle = 12  // circle the goal
};

// The functions and globals of maneuvers.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_jumpJetDrag;
	extern MechS32 g_slideSlope;
	extern Point g_probeDirections[16];
	extern ManeuverEntry g_mechManeuvers[13];
	extern ManeuverEntry g_stupidManeuvers;
	extern ManeuverEntry g_circleManeuvers;
	extern ManeuverEntry g_behindManeuvers;
	extern ManeuverEntry g_altMechManeuvers[13];
	extern MechS32 g_maneuverTablesReady;
	extern ManeuverTable g_maneuverTables[9];
	void RunManeuver(struct Player* p_player, MechU16 p_target);
	void InitializeManeuvers(struct Player* p_player);
	MechS32 ChooseManeuver(struct Player* p_player);
	MechS16 FindManeuver(ManeuverTable* p_table, MechS16 p_id);
	void StartManeuver(struct Player* p_player);
	void EndManeuver(struct Player* p_player);
	void PlaceOffsetNav(struct Player* p_player, MechU32 p_target, MechS16 p_direction, MechS16 p_distance);
	void GetOffsetPoint(
		MechU32 p_target,
		MechS16 p_direction,
		MechS32* p_x,
		MechS32* p_z,
		MechS32* p_y,
		MechS16 p_distance
	);
	MechS32 IsSharpTurn(struct Player* p_player, MechS32 p_turn);
	void ManeuverStupid(struct Player* p_player, MechS16 p_target);
	void ManeuverBehind(struct Player* p_player, MechS16 p_target);
	MechS32 ManeuverAchick(struct Player* p_player, MechS16 p_target);
	MechS32 ManeuverKama(struct Player* p_player, MechS16 p_target);
	MechS32 ManeuverWbackp(struct Player* p_player, MechS16 p_target);
	MechS32 ManeuverWchick(struct Player* p_player, MechS16 p_target);
	MechS32 ManeuverWpeek(struct Player* p_player, MechS16 p_target);
	MechS32 ManeuverAjmpin(struct Player* p_player, MechS16 p_target);
	MechS32 ManeuverSprint(struct Player* p_player, MechS16 p_target);
	MechS32 ManeuverAdfa(struct Player* p_player, MechS16 p_target);
	void ManeuverAvoid(struct Player* p_player, MechS16 p_target);
	MechS32 ManeuverCircle(struct Player* p_player, MechS16 p_target);
	MechS32 IsBelowHiddenTarget(struct Player* p_player);
	MechS32 BrakeFall(struct Player* p_player);
	void SetJumpJets(struct Player* p_player, MechS8 p_value);
	MechS32 AvoidObstacles(struct Player* p_player);
	MechS32 IsStandableShape(struct Shape* p_shape);
	void BuildProbeRay(
		struct Player* p_player,
		struct Ray* p_ray,
		MechS32 p_side,
		MechS16 p_step,
		MechS32 p_length,
		MechS32 p_fromEdge
	);
	MechS16 GetAvoidSide(struct Player* p_player, struct Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 GetHeadingTo(struct Player* p_player, MechS16 p_target);
	MechS32 ClampMagnitude(MechS32 p_value, MechS32 p_limit);
	MechS32 HasLineToTarget(struct Player* p_player, MechS32 p_y);
	MechS32 IsOutOfAmmo(struct Mech* p_mech);
	void SetJumpDirection(struct Player* p_player, MechS16 p_value);
	MechS32 GetClosingRate(struct Player* p_player);
	void JumpToTurn(struct Player* p_player);
	MechS32 CanJump(struct Player* p_player, MechS32 p_limit);
	struct Shape* GetTargetShape(MechS16 p_target);
	void DodgeShot(struct WeaponSlot* p_slot, struct Mech* p_mech);
	MechS16 ChooseFlankPlace(struct Player* p_player);
	MechS32 IsStuck(struct Mech* p_mech);

#ifdef __cplusplus
}
#endif

#endif // MANEUVERS_H
