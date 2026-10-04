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
typedef MechS32 (*PlayerCreatedFn)(MechS32 p_index, Player* p_player);

#pragma pack(push, 1)

// SIZE 0x1aa
struct Player {
	MechS32 m_type;        // 0x00 — c_playerType…
	MechS32 m_index;       // 0x04
	MechS32 m_team;        // 0x08
	MechS32 m_slot;        // 0x0c — the player's place in its team's formation
	MechS32 m_aiMode;      // 0x10 — the gpspec's ai: 0 the user, 2 the AI drives it
	MechS16 m_flags;       // 0x14
	MechS16 m_inspectedBy; // 0x16 — a bit per team that inspected it (m_inspectTarget)
	MechS32 m_baseLevel;   // 0x18 — the world stream's repeat pass it was made in (LoadBaseLevelShapes)
	MechS32 m_detailLevel; // 0x1c — the level of detail whose shapes are loaded, or -1
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
	MechS32 m_pitch;                 // 0x5c — 16.16 degrees, about x
	MechS32 m_heading;               // 0x60 — 16.16 degrees, about y
	MechS32 m_roll;                  // 0x64 — 16.16 degrees, about z
	MechS32 m_torsoPitch;            // 0x68 — the torso object's rotation, relative to m_obj
	MechS32 m_torsoTwist;            // 0x6c — added to the heading for the forward view
	MechS32 m_torsoRoll;             // 0x70
	MechS32 m_groundHeight;          // 0x74 — the terrain's under the mech
	MechS32 m_onGround;              // 0x78
	MechS32 m_collidedWith;          // 0x7c — the player its mech ran into this tick, or -1
	// 0x80: 0x1 animating, 0x2 the frame is a stride (the speed eases, MASC can fail), 0x4 airborne,
	// 0x8 the frame plays m_pendingSound, 0x10 restart the animation.
	MechU32 m_animFlags;                // 0x80
	MechS32 m_motionState;              // 0x84 — the animation state it reached, whose sound plays (gpanim.c)
	MechS32 m_nextMotionState;          // 0x88 — the one it wants: 0 moving, 1 at full throttle, 2 reversing, or -1
	MechS32 m_speedLevel;               // 0x8c — 0 to 3, from the throttle
	MechS32 m_pendingSound;             // 0x90 — a sound to play once, or -1
	MechS32 m_animRate;                 // 0x94
	Ramp m_aimRange;                    // 0x98 — eases towards m_aimDistance's
	Ramp m_aimDistance;                 // 0xa8 — the distance the weapons converge at
	MechS32 m_headingCos;               // 0xb8 — 16.16 (UpdateDoor)
	MechS32 m_headingSin;               // 0xbc — 16.16
	PlayerTargetInfo m_targetInfo;      // 0xc0
	MechChar m_name[0xfe - 0xe8];       // 0xe8
	MechChar m_shortName[0x114 - 0xfe]; // 0xfe — for the target panel
	MechS32 m_killer;                   // 0x114 — the player who destroyed its mech
	AiRule** m_rules;                   // 0x118 — the rules of the current state, NULL-terminated
	MechU16* m_ruleSets[3];             // 0x11c — AI scripts, by priority
	AiStackEntry m_stack[2];            // 0x128 — T_PUSH saves the state and goal in the first, QueueAIState behind it
	// 0x130: the gpspec record's AI parameters (BwdExecuteStream): gunnery, leash range, resting and
	// active alert ranges (in hundreds), piloting; InitializeAI turns them into the members below.
	MechU16 m_aiParams[8];           // 0x130
	MechU16 m_stackCount;            // 0x140
	undefined2 m_unk0x142;           // 0x142 — nothing reads or writes it
	PlayerAi m_ai;                   // 0x144
	MechS32 m_engageAtWill;          // 0x154 — it may be sent to attack: the "Engage at Will" order toggles it
	MechS8 m_gunnery;                // 0x158 — lower is better: it fires on one roll in m_gunnery
	MechS8 m_piloting;               // 0x159 — 1 (best) to 4: the maneuvers it may make
	MechS32 m_nextFireTime;          // 0x15a — the clock when it next rolls to fire
	MechS32 m_restAlertRange;        // 0x15e — the rules' range -1: enemies within it wake a resting player
	MechS32 m_alertRange;            // 0x162 — the rules' range -2: enemies within it are reported or engaged
	MechS32 m_leashRange;            // 0x166 — the rules' range -4: a chase this far from its anchor nav ends
	MechS32 m_nav;                   // 0x16a — a nav target id the AI placed, or 0x1000
	MechS16 m_nextDetectCheck;       // 0x16e — the clock (truncated) when IsTargetDetectable next looks
	MechS16 m_maneuver;              // 0x170 — c_maneuver…, or -1
	MechS16 m_lastManeuver;          // 0x172 — the previous maneuver, or -1
	MechS32 m_nextManeuver;          // 0x174 — one asked for (0 none), or -2 to flee
	MechS32 m_maneuverEnd;           // 0x178 — the clock when the maneuver ends, or 0
	MechS32 m_maneuverTimer;         // 0x17c — a clock time each maneuver sets its own way
	MechS32 m_maneuverParam;         // 0x180 — each maneuver's own: a place, side, step or count
	MechS32 m_maneuverFlag;          // 0x184 — each maneuver's own
	struct Shape* m_avoidShape;      // 0x188 — the shape AvoidObstacles steers around
	MechS32 m_nextAvoidCheck;        // 0x18c — the clock when AvoidObstacles next probes
	MechS16 m_avoidSide;             // 0x190 — 1 or -1 while avoiding, else 0
	MechS32 m_probeScale;            // 0x192 — 16.16, the probe rays' length from the speed
	MechS32 m_controlsJets;          // 0x196 — the maneuver works the jump jets itself
	MechS32 m_lastTargetDistance;    // 0x19a — at the last GetClosingRate
	MechU32 m_skillFlag0 : 1;        // 0x19e — the maneuvers its piloting allows (InitializeManeuvers)
	MechU32 m_skillFlag1 : 1;        // 0x19e
	MechU32 m_skillFlag2 : 1;        // 0x19e
	MechU32 m_skillFlag3 : 1;        // 0x19e
	MechU32 m_skillFlag4 : 1;        // 0x19e
	MechU32 m_skillFlag5 : 1;        // 0x19e
	MechU32 m_skillFlag6 : 1;        // 0x19e
	MechU32 m_skillFlagsUnused : 25; // 0x19e
	MechS8 m_placesTaken[8];         // 0x1a2 — the places around it (ChooseFlankPlace) that are taken
};

#pragma pack(pop)

// The functions and globals of players.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_playerCount;
	extern MechS32 g_gameThingCount;
	extern Player* g_players[];
	extern GameThing g_gameThings[254];

	void FirstClassFunctions(void);
	void UpdateAllPlayers(void);
	void LateUpdateAllPlayers(void);
	void UpdateLocalPlayer(void);
	void DrawLocalPlayer(void);
	void ShutdownAllPlayers(void);
	void ZeroGameThing(MechS32 p_index);
	void ZeroGamethings(void);
	void CreateSimPlayer(MechS32 p_player, PlayerCreatedFn p_fn);
	MechS32 AllocPlayer(MechS32 p_player);
	void InitPlayer(Player* p_player);

#ifdef __cplusplus
}
#endif

#endif // PLAYERS_H
