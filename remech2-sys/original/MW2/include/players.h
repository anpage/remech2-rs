#ifndef PLAYERS_H
#define PLAYERS_H

#include "aimessage.h"
#include "airule.h"
#include "aistackentry.h"
#include "decomp.h"
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
	int32_t m_type;        // 0x00 — c_playerType…
	int32_t m_index;       // 0x04
	int32_t m_team;        // 0x08
	int32_t m_slot;        // 0x0c — the player's place in its team's formation
	int32_t m_aiMode;      // 0x10 — the gpspec's ai: 0 the user, 2 the AI drives it
	int16_t m_flags;       // 0x14
	int16_t m_inspectedBy; // 0x16 — a bit per team that inspected it (m_inspectTarget)
	int32_t m_baseLevel;   // 0x18 — the world stream's repeat pass it was made in (LoadBaseLevelShapes)
	int32_t m_detailLevel; // 0x1c — the level of detail whose shapes are loaded, or -1
	Mech* m_mech;          // 0x20
	undefined4 m_mechSize; // 0x24 — sizeof(Mech) for the types that have one
	void (*m_firstClassFn)(Player* p_player); // 0x28
	PlayerMechFn m_updateFn;                  // 0x2c
	PlayerMechFn m_lateUpdateFn;              // 0x30
	PlayerMechFn m_localUpdateFn;             // 0x34
	PlayerMechFn m_drawFn;                    // 0x38
	PlayerMechFn m_shutdownFn;                // 0x3c
	struct SceneObject* m_obj;                // 0x40
	struct SceneObject* m_eyeObj;    // 0x44 — the world stream's eye object: the cockpit view and the weapons' aim
	struct SceneObject* m_firingObj; // 0x48 — the hardpoint of the weapon firing
	PlayerSteering* m_steering;      // 0x4c
	Vector3 m_position;              // 0x50
	int32_t m_pitch;                 // 0x5c — 16.16 degrees, about x
	int32_t m_heading;               // 0x60 — 16.16 degrees, about y
	int32_t m_roll;                  // 0x64 — 16.16 degrees, about z
	int32_t m_torsoPitch;            // 0x68 — the torso object's rotation, relative to m_obj
	int32_t m_torsoTwist;            // 0x6c — added to the heading for the forward view
	int32_t m_torsoRoll;             // 0x70
	int32_t m_groundHeight;          // 0x74 — the terrain's under the mech
	int32_t m_onGround;              // 0x78
	int32_t m_collidedWith;          // 0x7c — the player its mech ran into this tick, or -1
	// 0x80: 0x1 animating, 0x2 the frame is a stride (the speed eases, MASC can fail), 0x4 airborne,
	// 0x8 the frame plays m_pendingSound, 0x10 restart the animation.
	uint32_t m_animFlags;           // 0x80
	int32_t m_motionState;          // 0x84 — the animation state it reached, whose sound plays (gpanim.c)
	int32_t m_nextMotionState;      // 0x88 — the one it wants: 0 moving, 1 at full throttle, 2 reversing, or -1
	int32_t m_speedLevel;           // 0x8c — 0 to 3, from the throttle
	int32_t m_pendingSound;         // 0x90 — a sound to play once, or -1
	int32_t m_animRate;             // 0x94
	Ramp m_aimRange;                // 0x98 — eases towards m_aimDistance's
	Ramp m_aimDistance;             // 0xa8 — the distance the weapons converge at
	int32_t m_headingCos;           // 0xb8 — 16.16 (UpdateDoor)
	int32_t m_headingSin;           // 0xbc — 16.16
	PlayerTargetInfo m_targetInfo;  // 0xc0
	char m_name[0xfe - 0xe8];       // 0xe8
	char m_shortName[0x114 - 0xfe]; // 0xfe — for the target panel
	int32_t m_killer;               // 0x114 — the player who destroyed its mech
	AiRule** m_rules;               // 0x118 — the rules of the current state, NULL-terminated
	uint16_t* m_ruleSets[3];        // 0x11c — AI scripts, by priority
	AiStackEntry m_stack[2];        // 0x128 — T_PUSH saves the state and goal in the first, QueueAIState behind it
	// 0x130: the gpspec record's AI parameters (BwdExecuteStream): gunnery, leash range, resting and
	// active alert ranges (in hundreds), piloting; InitializeAI turns them into the members below.
	uint16_t m_aiParams[8];           // 0x130
	uint16_t m_stackCount;            // 0x140
	PlayerAi m_ai;                    // 0x144
	int32_t m_engageAtWill;           // 0x154 — it may be sent to attack: the "Engage at Will" order toggles it
	int8_t m_gunnery;                 // 0x158 — lower is better: it fires on one roll in m_gunnery
	int8_t m_piloting;                // 0x159 — 1 (best) to 4: the maneuvers it may make
	int32_t m_nextFireTime;           // 0x15a — the clock when it next rolls to fire
	int32_t m_restAlertRange;         // 0x15e — the rules' range -1: enemies within it wake a resting player
	int32_t m_alertRange;             // 0x162 — the rules' range -2: enemies within it are reported or engaged
	int32_t m_leashRange;             // 0x166 — the rules' range -4: a chase this far from its anchor nav ends
	int32_t m_nav;                    // 0x16a — a nav target id the AI placed, or 0x1000
	int16_t m_nextDetectCheck;        // 0x16e — the clock (truncated) when IsTargetDetectable next looks
	int16_t m_maneuver;               // 0x170 — c_maneuver…, or -1
	int16_t m_lastManeuver;           // 0x172 — the previous maneuver, or -1
	int32_t m_nextManeuver;           // 0x174 — one asked for (0 none), or -2 to flee
	int32_t m_maneuverEnd;            // 0x178 — the clock when the maneuver ends, or 0
	int32_t m_maneuverTimer;          // 0x17c — a clock time each maneuver sets its own way
	int32_t m_maneuverParam;          // 0x180 — each maneuver's own: a place, side, step or count
	int32_t m_maneuverFlag;           // 0x184 — each maneuver's own
	struct Shape* m_avoidShape;       // 0x188 — the shape AvoidObstacles steers around
	int32_t m_nextAvoidCheck;         // 0x18c — the clock when AvoidObstacles next probes
	int16_t m_avoidSide;              // 0x190 — 1 or -1 while avoiding, else 0
	int32_t m_probeScale;             // 0x192 — 16.16, the probe rays' length from the speed
	int32_t m_controlsJets;           // 0x196 — the maneuver works the jump jets itself
	int32_t m_lastTargetDistance;     // 0x19a — at the last GetClosingRate
	uint32_t m_skillFlag0 : 1;        // 0x19e — the maneuvers its piloting allows (InitializeManeuvers)
	uint32_t m_skillFlag1 : 1;        // 0x19e
	uint32_t m_skillFlag2 : 1;        // 0x19e
	uint32_t m_skillFlag3 : 1;        // 0x19e
	uint32_t m_skillFlag4 : 1;        // 0x19e
	uint32_t m_skillFlag5 : 1;        // 0x19e
	uint32_t m_skillFlag6 : 1;        // 0x19e
	uint32_t m_skillFlagsUnused : 25; // 0x19e
	int8_t m_placesTaken[8];          // 0x1a2 — the places around it (ChooseFlankPlace) that are taken
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
