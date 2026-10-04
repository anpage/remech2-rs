/* Reloading a player's mech: RememberLoadMech and RememberMechSegments save how it was
   loaded and its scene objects, and ReloadPlayerMech restores both. */
#include "mechreload.h"

#include "classtable.h"
#include "config.h"
#include "debris.h"
#include "debugprint.h"
#include "decomp.h"
#include "gpanim.h"
#include "mech.h"
#include "mechclass.h"
#include "mekfile.h"
#include "network.h"
#include "object.h"
#include "objective.h"
#include "players.h"
#include "rememberedmech.h"
#include "simmain.h"
#include "types.h"

#include <string.h>

// The player whose mech ReloadPlayerMech is reloading, or -1.
// GLOBAL: MW2 0x100ba690
MechS32 g_reloadingPlayer = -1;

// GLOBAL: MW2 0x100ba694
MechS32 g_unk0x100ba694 = 0;

// GLOBAL: MW2 0x100ba698
RememberedMech g_rememberedMechs[60] = {0};

// GLOBAL: MW2 0x100babc0
MechSegment* g_mechSegments[60] = {NULL};

// Reloads player p_player's mech as it was remembered. Without p_force, only a player with
// flag 2 set.
// FUNCTION: MW2 0x1007fbe0
MechS32 ReloadPlayerMech(MechS32 p_player, MechS32 p_force)
{
	Mech* mech;
	MechS16 flags;

	mech = NULL;
	if (p_player >= 0 && p_player < 60) {
		mech = g_players[p_player]->m_mech;
	}

	if (!mech) {
		return 0;
	}

	flags = g_players[p_player]->m_flags;
	if (!p_force && (!(flags & 2) || !(flags & 2))) {
		return 0;
	}

	mech->m_speed.m_target = mech->m_speed.m_value = 0;
	mech->m_torsoTwist.m_target = mech->m_torsoTwist.m_value = 0;
	mech->m_turnRate.m_target = mech->m_turnRate.m_value = 0;
	mech->m_torsoPitch.m_target = mech->m_torsoPitch.m_value = 0;
	mech->m_player->m_steering->m_throttle = 0;
	mech->m_player->m_steering->m_throttleSet = 1;
	mech->m_player->m_steering->m_jumpJetEnabled = 0;
	mech->m_player->m_steering->m_jumpJetFireLeft = 0;
	mech->m_player->m_steering->m_jumpJetFireRight = 0;
	mech->m_player->m_steering->m_jumpJetFireForward = 0;
	mech->m_player->m_steering->m_jumpJetFireBackward = 0;
	mech->m_player->m_steering->m_weaponFire = 0;
	mech->m_player->m_steering->m_weaponCycle = 0;
	mech->m_player->m_steering->m_legsPanMinus = 0;
	mech->m_player->m_steering->m_legsPanPlus = 0;
	mech->m_player->m_steering->m_reverse = 0;
	mech->m_player->m_steering->m_advanceNav = 0;
	mech->m_player->m_steering->m_autopilot = 0;

	g_reloadingPlayer = p_player;
	InitMechArrays(mech);
	LoadMechConfig(
		mech,
		g_rememberedMechs[p_player].m_name,
		g_rememberedMechs[p_player].m_id,
		g_rememberedMechs[p_player].m_config
	);
	mech->m_player->m_obj = RestoreMechSegments(g_mechSegments[p_player]);
	ShowObjTree(mech->m_player->m_obj);
	UpdateObj(mech->m_player->m_obj);
	mech->m_player->m_detailLevel = -1;
	mech->m_player->m_targetInfo.m_target = 0x1000;

	flags &= ~6;
	flags &= ~0x4001;
	flags |= 0x10;
	g_players[p_player]->m_flags = flags;

	if (g_isNetworkGame) {
		RestartStarMission(p_player);
	}

	// gpanim.c's Mech is the player
	ResetMotion(mech->m_player);
	FirstMech(mech->m_player);

	if (g_localPlayerId == p_player) {
		ResetCockpitPanels();
		g_localMechLost = 0;
		g_localMechDestroyed = 0;
		g_mechPoweredUp = 0;
	}

	g_reloadingPlayer = -1;
	return 1;
}

// FUNCTION: MW2 0x1007fecf
void RememberLoadMech(Mech* p_mech, MechChar* p_name, MechS32 p_id, MechChar* p_config)
{
	MechS32 id;

	if (!p_mech || !p_mech->m_player) {
		return;
	}

	id = p_mech->m_player->m_index;
	if (id > 60) {
		DebugPrint("RememberLoadMech(): gp_id > DEFAULT_GAMEPIECES\n");
		return;
	}

	g_rememberedMechs[id].m_id = p_id;
	strcpy(g_rememberedMechs[id].m_name, p_name);
	strcpy(g_rememberedMechs[id].m_config, p_config);
}

// FUNCTION: MW2 0x1007ff9c
void RememberMechSegments(Mech* p_mech)
{
	SceneObject* obj;
	MechS32 id;

	obj = NULL;
	if (!p_mech || !p_mech->m_player) {
		return;
	}

	id = p_mech->m_player->m_index;
	if (id > 60) {
		DebugPrint("RememberMechSegments(): gp_id > DEFAULT_GAMEPIECES\n");
		return;
	}

	obj = p_mech->m_player->m_obj;
	g_mechSegments[id] = SaveMechSegments(obj);
}

// FUNCTION: MW2 0x10080014
SceneObject* RestoreMechSegments(MechSegment* p_segment)
{
	SceneObject* obj;

	if (!p_segment) {
		return NULL;
	}

	obj = p_segment->m_obj;
	if (obj) {
		RemoveChunk(obj, ReleaseObjShape);
		ForgetObjShape(obj);
		SetObjPosition(obj, p_segment->m_position[0], p_segment->m_position[1], p_segment->m_position[2]);
		SetObjRotation(obj, p_segment->m_rotation[0], p_segment->m_rotation[1], p_segment->m_rotation[2], 0);
		obj->m_firstChild = RestoreMechSegments(p_segment->m_firstChild);
		obj->m_nextSibling = RestoreMechSegments(p_segment->m_nextSibling);
		obj->m_parent = p_segment->m_parent;
	}

	return obj;
}

// FUNCTION: MW2 0x100800e3
MechSegment* SaveMechSegments(SceneObject* p_obj)
{
	MechSegment* segment;

	segment = NULL;
	if (!p_obj) {
		return NULL;
	}

	segment = MechHeapAlloc(g_primaryHeap, sizeof(MechSegment));
	if (!segment) {
		return NULL;
	}

	segment->m_obj = p_obj;
	segment->m_parent = p_obj->m_parent;
	segment->m_firstChild = SaveMechSegments(p_obj->m_firstChild);
	segment->m_nextSibling = SaveMechSegments(p_obj->m_nextSibling);
	GetObjLocalPosition(p_obj, &segment->m_position[0], &segment->m_position[1], &segment->m_position[2]);
	GetObjAngles(p_obj, &segment->m_rotation[0], &segment->m_rotation[1], &segment->m_rotation[2]);

	return segment;
}
