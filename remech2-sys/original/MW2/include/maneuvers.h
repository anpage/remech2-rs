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

	extern int32_t g_jumpJetDrag;
	extern int32_t g_slideSlope;
	extern Point g_probeDirections[16];
	extern ManeuverEntry g_mechManeuvers[13];
	extern ManeuverEntry g_stupidManeuvers;
	extern ManeuverEntry g_circleManeuvers;
	extern ManeuverEntry g_behindManeuvers;
	extern ManeuverEntry g_altMechManeuvers[13];
	extern int32_t g_maneuverTablesReady;
	extern ManeuverTable g_maneuverTables[9];
	void RunManeuver(struct Player* p_player, uint16_t p_target);
	void InitializeManeuvers(struct Player* p_player);
	int32_t ChooseManeuver(struct Player* p_player);
	int16_t FindManeuver(ManeuverTable* p_table, int16_t p_id);
	void StartManeuver(struct Player* p_player);
	void EndManeuver(struct Player* p_player);
	void PlaceOffsetNav(struct Player* p_player, uint32_t p_target, int16_t p_direction, int16_t p_distance);
	void GetOffsetPoint(
		uint32_t p_target,
		int16_t p_direction,
		int32_t* p_x,
		int32_t* p_z,
		int32_t* p_y,
		int16_t p_distance
	);
	int32_t IsSharpTurn(struct Player* p_player, int32_t p_turn);
	void ManeuverStupid(struct Player* p_player, int16_t p_target);
	void ManeuverBehind(struct Player* p_player, int16_t p_target);
	int32_t ManeuverAchick(struct Player* p_player, int16_t p_target);
	int32_t ManeuverKama(struct Player* p_player, int16_t p_target);
	int32_t ManeuverWbackp(struct Player* p_player, int16_t p_target);
	int32_t ManeuverWchick(struct Player* p_player, int16_t p_target);
	int32_t ManeuverWpeek(struct Player* p_player, int16_t p_target);
	int32_t ManeuverAjmpin(struct Player* p_player, int16_t p_target);
	int32_t ManeuverSprint(struct Player* p_player, int16_t p_target);
	int32_t ManeuverAdfa(struct Player* p_player, int16_t p_target);
	void ManeuverAvoid(struct Player* p_player, int16_t p_target);
	int32_t ManeuverCircle(struct Player* p_player, int16_t p_target);
	int32_t IsBelowHiddenTarget(struct Player* p_player);
	int32_t BrakeFall(struct Player* p_player);
	void SetJumpJets(struct Player* p_player, int8_t p_value);
	int32_t AvoidObstacles(struct Player* p_player);
	int32_t IsStandableShape(struct Shape* p_shape);
	void BuildProbeRay(
		struct Player* p_player,
		struct Ray* p_ray,
		int32_t p_side,
		int16_t p_step,
		int32_t p_length,
		int32_t p_fromEdge
	);
	int16_t GetAvoidSide(struct Player* p_player, struct Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t GetHeadingTo(struct Player* p_player, int16_t p_target);
	int32_t ClampMagnitude(int32_t p_value, int32_t p_limit);
	int32_t HasLineToTarget(struct Player* p_player, int32_t p_y);
	int32_t IsOutOfAmmo(struct Mech* p_mech);
	void SetJumpDirection(struct Player* p_player, int16_t p_value);
	int32_t GetClosingRate(struct Player* p_player);
	void JumpToTurn(struct Player* p_player);
	int32_t CanJump(struct Player* p_player, int32_t p_limit);
	struct Shape* GetTargetShape(int16_t p_target);
	void DodgeShot(struct WeaponSlot* p_slot, struct Mech* p_mech);
	int16_t ChooseFlankPlace(struct Player* p_player);
	int32_t IsStuck(struct Mech* p_mech);

#ifdef __cplusplus
}
#endif

#endif // MANEUVERS_H
