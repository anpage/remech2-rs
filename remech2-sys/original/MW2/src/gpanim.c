#include "gpanim.h"

#include "decomp.h"
#include "eyepoint.h"
#include "mech.h"
#include "players.h"
#include "playersteering.h"
#include "polydraw.h"
#include "simmain.h"
#include "soundfx.h"
#include "types.h"

int32_t g_motionSounds[4][4] =
	{{-1, 0x12e, 0x12e, -1}, {0x14f, -1, -1, 0x14f}, {0x11a, 0x11a, 0x148, 0x11a}, {0x119, 0x119, 0x119, 0x12d}};

int32_t g_lastMotionSound[60];

void FirstGPAnim(void)
{
	int32_t i;

	for (i = 0; i < 60; i++) {
		g_lastMotionSound[i] = -2;
	}
}

void StartMotion(Player* p_player)
{
	if (p_player->m_index != g_localPlayerId) {
		p_player->m_pendingSound = 0x103;
	}

	p_player->m_speedLevel = 0;
	p_player->m_nextMotionState = -1;
}

void StopMotion(Player* p_player)
{
	p_player->m_speedLevel = 0;
	p_player->m_nextMotionState = -1;
}

int32_t* GetEyepointOffset(Player* p_player, int32_t* p_offset)
{
	p_offset[0] = g_eyepoint->m_x - p_player->m_position.m_x;
	p_offset[1] = g_eyepoint->m_y - p_player->m_position.m_y;
	p_offset[2] = g_eyepoint->m_z - p_player->m_position.m_z;
	return p_offset;
}

// Picks the player's motion state from its mech's throttle (m_speedLevel, m_nextMotionState; 2
// while its steering's m_reverse is set), plays its pending sound m_pendingSound once (bit 8 of
// m_animFlags), and for the local player updates the looping sounds (UpdateMotionSounds).
void UpdateMotion(Player* p_player)
{
	int32_t* offset;
	int32_t position[3];
	int32_t height;

	offset = NULL;
	if (p_player->m_mech->m_throttle.m_value <= 0x420) {
		StopMotion(p_player);
	}
	else {
		height = p_player->m_mech->m_throttle.m_value - 0x400;
		if (height < 0) {
			height = 0;
		}

		p_player->m_nextMotionState = 0;
		if (height < 0x100) {
			p_player->m_speedLevel = 1;
		}
		else if (height < 0x300) {
			p_player->m_speedLevel = 2;
		}
		else {
			p_player->m_speedLevel = 3;
			p_player->m_nextMotionState = 1;
		}

		if (p_player->m_motionState == 2 && p_player->m_speedLevel > 2) {
			p_player->m_speedLevel = 2;
		}

		if (p_player->m_steering->m_reverse) {
			p_player->m_nextMotionState = 2;
		}

		if (p_player->m_type == c_playerTypeTank) {
			p_player->m_motionState = p_player->m_nextMotionState;
		}
	}

	if (p_player->m_pendingSound != -1 && p_player->m_animFlags & 8) {
		p_player->m_animFlags &= ~8;
		offset = GetEyepointOffset(p_player, position);
		PlaySoundAt(position[0], position[1], position[2], p_player->m_pendingSound, g_inCockpitView);
	}

	if (p_player->m_index == g_localPlayerId) {
		UpdateMotionSounds(p_player, g_motionSounds, offset);
	}
}

void UpdateMotionSounds(Player* p_player, int32_t (*p_sounds)[4], int32_t* p_offset)
{
	int32_t offset[3];
	int32_t sound;
	int32_t id;

	id = p_player->m_index;
	if ((p_player->m_motionState != p_player->m_nextMotionState && g_lastMotionSound[id] != p_player->m_motionState) ||
		(p_player->m_motionState == p_player->m_nextMotionState && g_lastMotionSound[id] != -2)) {
		if (p_player->m_motionState == 0 && g_lastMotionSound[id] == 2) {
			sound = p_sounds[0][1];
		}
		else {
			sound = p_sounds[p_player->m_motionState + 1][p_player->m_nextMotionState + 1];
		}

		if (sound != -1) {
			if (p_offset == NULL) {
				p_offset = GetEyepointOffset(p_player, offset);
			}

			PlaySoundAt(p_offset[0], p_offset[1], p_offset[2], sound, g_inCockpitView);
		}

		if (p_player->m_motionState != p_player->m_nextMotionState) {
			g_lastMotionSound[id] = p_player->m_motionState;
		}
		else {
			g_lastMotionSound[id] = -2;
		}
	}
}

void ResetMotion(Player* p_player)
{
	int32_t id;

	id = p_player->m_index;
	p_player->m_speedLevel = 0;
	p_player->m_nextMotionState = -1;
	p_player->m_motionState = -1;
	p_player->m_animFlags |= 0x10;
	g_lastMotionSound[id] = -2;
}
