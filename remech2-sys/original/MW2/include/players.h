#ifndef PLAYERS_H
#define PLAYERS_H

#include "aimessage.h"
#include "airule.h"
#include "aistackentry.h"
#include "gamething.h"
#include "mech.h"
#include "playerai.h"
#include "playersteering.h"
#include "playertargetinfo.h"
#include "ramp.h"
#include "types.h"
#include "vector3.h"

struct SceneObject;
struct Shape;
typedef struct Player Player;

typedef void (*PlayerMechFn)(Mech* p_mech);

// The player types (Player::m_type): the gpspec's GP_MW2… names (g_playerTypeNames).
enum {
	c_playerTypeNull = 0,
	c_playerTypeMech = 1,
	c_playerTypeTruck = 2,
	c_playerTypeArtillery = 3,
	c_playerTypeTank = 4,
	c_playerTypeHelicopter = 5,
	c_playerTypeWanderer = 6,
	c_playerTypeDoor = 7
};
typedef int32_t (*PlayerCreatedFn)(int32_t p_index, Player* p_player);

#pragma pack(push, 1)

struct Player {
	int32_t m_type; // c_playerType…
	int32_t m_index;
	int32_t m_team;
	int32_t m_slot;   // the player's place in its team's formation
	int32_t m_aiMode; // the gpspec's ai: 0 the user, 2 the AI drives it
	int16_t m_flags;
	int16_t m_inspectedBy; // a bit per team that inspected it (m_inspectTarget)
	int32_t m_baseLevel;   // the world stream's repeat pass it was made in (LoadBaseLevelShapes)
	int32_t m_detailLevel; // the level of detail whose shapes are loaded, or -1
	Mech* m_mech;
	uint32_t m_mechSize; // sizeof(Mech) for the types that have one
	void (*m_firstClassFn)(Player* p_player);
	PlayerMechFn m_updateFn;
	PlayerMechFn m_lateUpdateFn;
	PlayerMechFn m_localUpdateFn;
	PlayerMechFn m_drawFn;
	PlayerMechFn m_shutdownFn;
	struct SceneObject* m_obj;
	struct SceneObject* m_eyeObj;    // the world stream's eye object: the cockpit view and the weapons' aim
	struct SceneObject* m_firingObj; // the hardpoint of the weapon firing
	PlayerSteering* m_steering;
	Vector3 m_position;
	int32_t m_pitch;      // 16.16 degrees, about x
	int32_t m_heading;    // 16.16 degrees, about y
	int32_t m_roll;       // 16.16 degrees, about z
	int32_t m_torsoPitch; // the torso object's rotation, relative to m_obj
	int32_t m_torsoTwist; // added to the heading for the forward view
	int32_t m_torsoRoll;
	int32_t m_groundHeight; // the terrain's under the mech
	int32_t m_onGround;
	int32_t m_collidedWith; // the player its mech ran into this tick, or -1
	// 0x1 animating, 0x2 the frame is a stride (the speed eases, MASC can fail), 0x4 airborne,
	// 0x8 the frame plays m_pendingSound, 0x10 restart the animation.
	uint32_t m_animFlags;
	int32_t m_motionState;     // the animation state it reached, whose sound plays (gpanim.c)
	int32_t m_nextMotionState; // the one it wants: 0 moving, 1 at full throttle, 2 reversing, or -1
	int32_t m_speedLevel;      // 0 to 3, from the throttle
	int32_t m_pendingSound;    // a sound to play once, or -1
	int32_t m_animRate;
	Ramp m_aimRange;      // eases towards m_aimDistance's
	Ramp m_aimDistance;   // the distance the weapons converge at
	int32_t m_headingCos; // 16.16 (UpdateDoor)
	int32_t m_headingSin; // 16.16
	PlayerTargetInfo m_targetInfo;
	char m_name[0xfe - 0xe8];
	char m_shortName[0x114 - 0xfe]; // for the target panel
	int32_t m_killer;               // the player who destroyed its mech
	AiRule** m_rules;               // the rules of the current state, NULL-terminated
	uint16_t* m_ruleSets[3];        // AI scripts, by priority
	AiStackEntry m_stack[2];        // T_PUSH saves the state and goal in the first, QueueAIState behind it
	// The gpspec record's AI parameters (BwdExecuteStream): gunnery, leash range, resting and
	// active alert ranges (in hundreds), piloting; InitializeAI turns them into the members below.
	uint16_t m_aiParams[8];
	uint16_t m_stackCount;
	PlayerAi m_ai;
	int32_t m_engageAtWill;       // it may be sent to attack: the "Engage at Will" order toggles it
	int8_t m_gunnery;             // lower is better: it fires on one roll in m_gunnery
	int8_t m_piloting;            // 1 (best) to 4: the maneuvers it may make
	int32_t m_nextFireTime;       // the clock when it next rolls to fire
	int32_t m_restAlertRange;     // the rules' range -1: enemies within it wake a resting player
	int32_t m_alertRange;         // the rules' range -2: enemies within it are reported or engaged
	int32_t m_leashRange;         // the rules' range -4: a chase this far from its anchor nav ends
	int32_t m_nav;                // a nav target id the AI placed, or 0x1000
	int16_t m_nextDetectCheck;    // the clock (truncated) when IsTargetDetectable next looks
	int16_t m_maneuver;           // c_maneuver…, or -1
	int16_t m_lastManeuver;       // the previous maneuver, or -1
	int32_t m_nextManeuver;       // one asked for (0 none), or -2 to flee
	int32_t m_maneuverEnd;        // the clock when the maneuver ends, or 0
	int32_t m_maneuverTimer;      // a clock time each maneuver sets its own way
	int32_t m_maneuverParam;      // each maneuver's own: a place, side, step or count
	int32_t m_maneuverFlag;       // each maneuver's own
	struct Shape* m_avoidShape;   // the shape AvoidObstacles steers around
	int32_t m_nextAvoidCheck;     // the clock when AvoidObstacles next probes
	int16_t m_avoidSide;          // 1 or -1 while avoiding, else 0
	int32_t m_probeScale;         // 16.16, the probe rays' length from the speed
	int32_t m_controlsJets;       // the maneuver works the jump jets itself
	int32_t m_lastTargetDistance; // at the last GetClosingRate
	uint32_t m_skillFlag0 : 1;    // the maneuvers its piloting allows (InitializeManeuvers)
	uint32_t m_skillFlag1 : 1;
	uint32_t m_skillFlag2 : 1;
	uint32_t m_skillFlag3 : 1;
	uint32_t m_skillFlag4 : 1;
	uint32_t m_skillFlag5 : 1;
	uint32_t m_skillFlag6 : 1;
	uint32_t m_skillFlagsUnused : 25;
	int8_t m_placesTaken[8]; // the places around it (ChooseFlankPlace) that are taken
};

#pragma pack(pop)

// The functions and globals of players.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_playerCount;
	extern int32_t g_gameThingCount;
	extern Player* g_players[0x3c];
	extern GameThing g_gameThings[254];

	void FirstClassFunctions(void);
	void UpdateAllPlayers(void);
	void LateUpdateAllPlayers(void);
	void UpdateLocalPlayer(void);
	void DrawLocalPlayer(void);
	void ShutdownAllPlayers(void);
	void ZeroGameThing(int32_t p_index);
	void ZeroGamethings(void);
	void CreateSimPlayer(int32_t p_player, PlayerCreatedFn p_fn);
	int32_t AllocPlayer(int32_t p_player);
	void InitPlayer(Player* p_player);

#ifdef __cplusplus
}
#endif

#endif // PLAYERS_H
