#include "mechclass.h"

#include "ai.h"
#include "ammobin.h"
#include "approxlen.h"
#include "clock.h"
#include "collision.h"
#include "config.h"
#include "decomp.h"
#include "environment.h"
#include "eyepoint.h"
#include "fadepal.h"
#include "fixeddiv.h"
#include "fixeddiv29.h"
#include "fixedmul.h"
#include "fixedmul29.h"
#include "fixedtrig.h"
#include "gamekeys.h"
#include "gpanim.h"
#include "inputmap.h"
#include "integrate.h"
#include "maneuvers.h"
#include "mech.h"
#include "mechcollision.h"
#include "mechdamage.h"
#include "mechreload.h"
#include "mechsection.h"
#include "muldiv.h"
#include "network.h"
#include "object.h"
#include "objective.h"
#include "pausebanner.h"
#include "players.h"
#include "playersteering.h"
#include "polydraw.h"
#include "poolsizes.h"
#include "ramp.h"
#include "random.h"
#include "ray.h"
#include "resource.h"
#include "shape.h"
#include "simmain.h"
#include "soundfx.h"
#include "speech.h"
#include "staticmem.h"
#include "targeting.h"
#include "team.h"
#include "timedoverlays.h"
#include "types.h"
#include "weapons.h"
#include "weaponslot.h"

#include <stdio.h>

DECOMP_SIZE_ASSERT(Mech, 0x10e)

// The "mightymouse" cheat: infinite jump jet fuel.
// GLOBAL: MW2 0x100a2be4
MechS32 g_infiniteJumpFuel = 0;

// Set by a game key: run JettisonAmmo on the local mech next tick.
// GLOBAL: MW2 0x100a2be8
MechS32 g_jettisonAmmoRequested = 0;

// Set by the MASC game key: toggle the local mech's MASC next tick.
// GLOBAL: MW2 0x100a2bec
MechS32 g_toggleMascRequested = 0;

// GLOBAL: MW2 0x100a2bf0
MechS32 g_mascEngaged = 0;

// GLOBAL: MW2 0x100a2bf4
MechS32 g_unk0x100a2bf4 = 0;

// GLOBAL: MW2 0x100a2bf8
MechS32 g_manualWeaponCycle = 0;

// Set when the local mech lands while g_unk0x100a2c00 is on (UpdateMech).
// GLOBAL: MW2 0x100a2bfc
MechS32 g_unk0x100a2bfc = 0;

// GLOBAL: MW2 0x100a2c00
MechS32 g_unk0x100a2c00 = 1;

// GLOBAL: MW2 0x100a2c04
MechS32 g_localMechLost = 0;

// A power request for the local mech: 1 powers it up, -1 shuts it down.
// GLOBAL: MW2 0x100a2c08
MechS32 g_powerRequest = 0;

// The local player's heading at the last tick, while the torso recenters (LateUpdateMech).
// GLOBAL: MW2 0x100a2c0c
MechS32 g_recenterLastHeading = 0;

// GLOBAL: MW2 0x100a2c10
MechS32 g_mechPoweredUp = 0;

// The clock at the last MASC malfunction roll (LateUpdateMech).
// GLOBAL: MW2 0x100a2c14
MechS32 g_lastMascRoll = 0;

// GLOBAL: MW2 0x100a2c18
MechS32 g_localMechDestroyed = 0;

// GLOBAL: MW2 0x100a2c1c
MechS32 g_ejectStarted = 0;

// Set once the local mech's collision sound played; cleared when it moves freely (UpdateMech).
// GLOBAL: MW2 0x100be00c
MechS32 g_collisionSoundPlayed;

// Puts p_player's mech back in its starting state: fresh parts for a new mech (and, for the local
// player, its cockpit), the ramps, weapon and motion state, its object back on the ground and the
// player's pose from it. With g_startOnAutopilot the local player starts on the autopilot.
// FUNCTION: MW2 0x10016ad0
void FirstMech(struct Player* p_player)
{
	Mech* mech;

	mech = p_player->m_mech;
	if (!mech) {
		return;
	}

	if (p_player->m_index != g_reloadingPlayer) {
		mech->m_torsoObj = NextThingRecordObject();
		mech->m_pitchObj = NextThingRecordObject();
		RememberMechSegments(mech);
		if (mech->m_player->m_index == g_localPlayerId) {
			InitCockpitPanels();
		}
	}

	if (mech->m_player->m_index == g_localPlayerId || g_isNetworkGame) {
		StartRamp(&mech->m_torsoTwist, 0, 0, 0.2);
	}
	else {
		StartRamp(&mech->m_torsoTwist, 0, 0, 0.6);
	}

	StartRamp(&mech->m_speed, 0, 0, 0.2);
	StartRamp(&mech->m_turnRate, 0, 0, 0.3);
	StartRamp(&mech->m_torsoPitch, 0, 0, 0.2);
	StartRamp(&mech->m_throttle, 0x400, 0x400, 0.2);
	mech->m_selectedWeapon = 0;
	mech->m_heat = 0;
	mech->m_lastSelectedWeapon = 0;
	mech->m_collisionTicks = 0;
	mech->m_flags |= 0x2000;
	mech->m_powerState = 0;
	mech->m_stateTime = 0;
	mech->m_unk0x90 = 0;
	mech->m_deltaHeat = 0;
	mech->m_unk0xb4 = 0;
	mech->m_autopilot = 0;
	mech->m_velocityX = 0;
	mech->m_velocityY = 0;
	mech->m_velocityZ = 0;
	mech->m_newVelocityX = 0;
	mech->m_newVelocityY = 0;
	mech->m_newVelocityZ = 0;
	mech->m_unk0xf0 = 0;
	mech->m_mobility = 0x10000;
	MoveObj(mech->m_player->m_obj, 0, mech->m_height, 0);
	UpdateObj(mech->m_player->m_obj);
	GetObjWorldAngles(
		mech->m_player->m_obj,
		&mech->m_player->m_pitch,
		&mech->m_player->m_heading,
		&mech->m_player->m_roll
	);
	GetObjPosition(
		mech->m_player->m_obj,
		&mech->m_player->m_position.m_x,
		&mech->m_player->m_position.m_y,
		&mech->m_player->m_position.m_z
	);
	mech->m_player->m_torsoPitch = mech->m_player->m_torsoTwist = mech->m_player->m_torsoRoll = 0;
	mech->m_player->m_steering->m_advanceTarget = 0;
	mech->m_player->m_steering->m_previousTarget = 0;
	mech->m_player->m_steering->m_resetTarget = 0;
	mech->m_player->m_steering->m_nearestEnemy = 0;
	mech->m_player->m_steering->m_nextObjective = 0;
	StartMotion(mech->m_player);
	EnableObjTreeCollision(mech->m_player->m_obj);
	if (g_startOnAutopilot && mech->m_player->m_index == g_localPlayerId) {
		mech->m_player->m_steering->m_throttle = 0x333;
		mech->m_player->m_steering->m_autopilot = 1;
		mech->m_player->m_steering->m_advanceNav = 1;
		g_autopilotStart = 1;
	}
	else {
		mech->m_player->m_steering->m_autopilot = 0;
		mech->m_player->m_steering->m_throttle = 0;
		mech->m_player->m_steering->m_advanceNav = 0;
	}

	InitializeAI(mech->m_player);
	mech->m_topSpeed = FixedDiv16(mech->m_topSpeed, g_gravityScale);
}

// Moves p_mech for the tick: eases its ramps, integrates its velocity (towards the speed it is
// driven at on the ground, or under gravity, drag and jump jet thrust in the air), resolves
// collisions with shapes and other mechs, lands it on the terrain (with falling damage), slides
// it down slopes, then poses its objects. Every player type's update (PlayerType::m_updateFn).
// Stack-slot permutation of the locals (its wider [ebp-N] encodings also shift the jumps).
// FUNCTION: MW2 0x10016edf
void UpdateMech(Mech* p_mech)
{
	MechS32 height;
	MechS32 accelY;
	MechS32 velZ;
	MechS32 posZ;
	MechS32 dy;
	MechS32 isLocal;
	Player* hitPlayer;
	MechS32 speed;
	MechS32 length;
	MechS32 dragX;
	MechS32 accelZ;
	MechS32 dz;
	struct Shape* hitShape;
	MechS32 dragZ;
	MechS32 targetX;
	MechS32 topSpeed;
	MechS32 drag;
	Mech* mech;
	MechS32 heading;
	MechS32 targetZ;
	MechS32 pitch;
	MechS32 impact;
	MechS32 velX;
	MechS32 posX;
	MechS32 accelX;
	MechS32 velY;
	MechS32 posY;
	MechS32 dx;
	MechS32 a;
	MechS32 b;
	MechS32 c;
	MechS32 topSpeed2;
	MechS32 posX2;
	MechS32 posY2;
	MechS32 posZ2;
	struct Shape* hitShape2;
	MechS32 savedUnk0xa4;
	MechS32 sound;
	MechS32 volume;
	MechS32 sound2;
	MechS32 volume2;
	MechS32 damage;
	MechS32 slope;
	MechS32 objY;
	MechS32 angle;
	MechS32 objZ;
	MechS32 rayLength;
	Ray ray;
	MechS32 objX;

	height = 0;
	hitShape = NULL;
	hitPlayer = NULL;

	if (!p_mech) {
		return;
	}

	mech = p_mech;
	if ((mech->m_powerState == 4 && mech->m_player->m_onGround) || (mech->m_flags & 0x100)) {
		return;
	}

	mech->m_deltaHeat = 0;
	isLocal = mech->m_player->m_index == g_localPlayerId;

	if (!(mech->m_player->m_flags & 1)) {
		UpdateRamp(&mech->m_torsoTwist);
		UpdateRamp(&mech->m_torsoPitch);
		UpdateRamp(&mech->m_turnRate);
		UpdateRamp(&mech->m_throttle);
		UpdateRamp(&mech->m_speed);

		velX = mech->m_velocityX;
		velY = mech->m_velocityY;
		velZ = mech->m_velocityZ;
		accelX = 0;
		accelY = -g_gravity;
		accelZ = 0;
		dragX = 0;
		dragZ = 0;

		if (mech->m_player->m_onGround && (!mech->m_player->m_steering->m_jumpJetEnabled || mech->m_jumpFuel <= 0)) {
			speed = mech->m_speed.m_value;
			if (speed < 0x20 && speed > -0x20) {
				speed = 0;
			}

			targetX = FixedMul16(speed, mech->m_player->m_headingSin);
			targetZ = FixedMul16(speed, mech->m_player->m_headingCos);
			if (g_deltaTime < 0x2d) {
				accelX = (targetX - mech->m_velocityX) / 0x2d;
				accelZ = (targetZ - mech->m_velocityZ) / 0x2d;
			}
			else if (g_deltaTime > 0) {
				accelX = (targetX - mech->m_velocityX) / g_deltaTime;
				accelZ = (targetZ - mech->m_velocityZ) / g_deltaTime;
			}
			else {
				accelX = 0;
				accelZ = 0;
			}
		}
		else {
			length = ApproximateVectorLength(velX, 0, velZ);
			topSpeed = MulDiv64(0x1400, mech->m_topSpeed, 2);
			if (length > 0 && topSpeed > 0 && mech->m_jumpThrust > 0) {
				topSpeed2 = FixedMul16(topSpeed, topSpeed);
				a = mech->m_jumpThrust - FixedDiv16(mech->m_jumpThrust, topSpeed) / 3;
				c = topSpeed2 - topSpeed * 3 / 4;
				a = FixedDiv16(a, c);
				b = mech->m_jumpThrust - FixedMul16(a, topSpeed2);
				b = FixedDiv16(b, topSpeed);
				drag = -FixedMul16(b + FixedMul16(a, length), length);
				dragX = MulDiv64(drag, velX, length);
				dragZ = MulDiv64(drag, velZ, length);
			}
			else {
				dragX = 0;
				dragZ = 0;
			}
		}

		drag = 0;
		dx = dy = dz = 0;

		if (mech->m_player->m_steering->m_jumpJetEnabled && mech->m_jumpFuel > 0 && mech->m_powerState == 2) {
			velX = mech->m_velocityX;
			velZ = mech->m_velocityZ;
			if (velY > 0) {
				drag = FixedMul16((g_gravity - mech->m_jumpThrust) * 0xe24, velY) / g_jumpJetDrag;
			}

			accelY += mech->m_jumpThrust + drag;
			if (mech->m_player->m_steering->m_turn) {
				if (velY <= 0) {
					accelY = 0;
				}
				else {
					accelY = -g_gravity;
				}
			}
			else if (mech->m_player->m_steering->m_jumpJetFireLeft) {
				if (velY <= 0) {
					accelY = 0;
				}
				else {
					accelY = -g_gravity;
				}

				accelX += -FixedMul16(mech->m_player->m_headingCos, mech->m_jumpThrust) + dragX;
				accelZ += dragZ + FixedMul16(mech->m_player->m_headingSin, mech->m_jumpThrust);
			}
			else if (mech->m_player->m_steering->m_jumpJetFireRight) {
				if (velY <= 0) {
					accelY = 0;
				}
				else {
					accelY = -g_gravity;
				}

				accelX += dragX + FixedMul16(mech->m_player->m_headingCos, mech->m_jumpThrust);
				accelZ += -FixedMul16(mech->m_player->m_headingSin, mech->m_jumpThrust) + dragZ;
			}
			else if (mech->m_player->m_steering->m_jumpJetFireForward) {
				if (velY <= 0) {
					accelY = 0;
				}
				else {
					accelY = -g_gravity;
				}

				accelX += dragX + FixedMul16(mech->m_player->m_headingSin, mech->m_jumpThrust);
				accelZ += dragZ + FixedMul16(mech->m_player->m_headingCos, mech->m_jumpThrust);
			}
			else if (mech->m_player->m_steering->m_jumpJetFireBackward) {
				if (velY <= 0) {
					accelY = 0;
				}
				else {
					accelY = -g_gravity;
				}

				accelX += -FixedMul16(mech->m_player->m_headingSin, mech->m_jumpThrust) + dragX;
				accelZ += -FixedMul16(mech->m_player->m_headingCos, mech->m_jumpThrust) + dragZ;
			}
		}
		else if (velY == 0 && mech->m_player->m_onGround) {
			accelY = 0;
		}

		IntegrateMidpoint(&dx, &velX, accelX, g_deltaTime);
		IntegrateMidpoint(&dy, &velY, accelY, g_deltaTime);
		IntegrateMidpoint(&dz, &velZ, accelZ, g_deltaTime);
		if (velX < 0x1000 && velX > -0x1000) {
			velX = 0;
		}
		if (velZ < 0x1000 && velZ > -0x1000) {
			velZ = 0;
		}

		mech->m_newVelocityX = velX;
		mech->m_newVelocityY = velY;
		mech->m_newVelocityZ = velZ;
		posX = mech->m_player->m_position.m_x;
		posY = mech->m_player->m_position.m_y;
		posZ = mech->m_player->m_position.m_z;
		hitPlayer = NULL;
		mech->m_player->m_collidedWith = -1;

		if (MoveMechWithCollisions(mech, &hitShape, &hitPlayer, dx, dy, dz, &posX, &posY, &posZ)) {
			dx = posX - mech->m_player->m_position.m_x;
			dy = posY - mech->m_player->m_position.m_y;
			dz = posZ - mech->m_player->m_position.m_z;
			velX = mech->m_velocityX;
			velY = mech->m_velocityY;
			velZ = mech->m_velocityZ;
			savedUnk0xa4 = mech->m_collisionTicks;

			if (MoveMechWithCollisions(mech, &hitShape2, &hitPlayer, dx, dy, dz, &posX2, &posY2, &posZ2)) {
				if (hitShape2 != hitShape) {
					posX = mech->m_player->m_position.m_x;
					posY = mech->m_player->m_position.m_y;
					posZ = mech->m_player->m_position.m_z;
				}

				velX = mech->m_velocityX = velX * 2;
				velY = mech->m_velocityY = velY * 2;
				velZ = mech->m_velocityZ = velZ * 2;
			}

			mech->m_collisionTicks = savedUnk0xa4;
			if (mech->m_powerState == 4) {
				KnockMechOver(mech);
				mech->m_player->m_onGround = 1;
				return;
			}

			g_groundNormalX = g_groundNormalY = g_groundNormalZ = 0;
			mech->m_player->m_groundHeight = GetTerrainHeight(posX, posY, posZ);
			height = posY - mech->m_height - mech->m_player->m_groundHeight;

			if (g_isNetworkGame && g_segmentNormalY > 0xddb4 && hitPlayer) {
				SendCollisionMsg(hitPlayer->m_index, g_segmentNormalX, g_segmentNormalY, g_segmentNormalZ);
				if (isLocal && g_inCockpitView && !g_collisionSoundPlayed) {
					g_collisionSoundPlayed = 1;
					impact = ApproximateVectorLength(mech->m_newVelocityX, mech->m_newVelocityY, mech->m_newVelocityZ);
					if (impact > 200000) {
						sound2 = 0xf0;
						volume2 = impact;
						if (volume2 > 1500000) {
							volume2 = 1500000;
						}

						volume2 = MulDiv64(200, volume2, 1500000);
						PlaySoundEffect(sound2, volume2, 0x40, 5, 0x32);
					}
				}
			}
			else if (g_segmentNormalY > 0xb505 && height < 1000 && height > -10000) {
				mech->m_collisionTicks = 0;
				height = -1;
			}
			else {
				if (isLocal && g_inCockpitView && !g_collisionSoundPlayed) {
					g_collisionSoundPlayed = 1;
					impact = ApproximateVectorLength(mech->m_newVelocityX, mech->m_newVelocityY, mech->m_newVelocityZ);
					if (impact > 200000) {
						if (hitPlayer) {
							if (g_isNetworkGame) {
								SendCollisionMsg(
									hitPlayer->m_index,
									g_segmentNormalX,
									g_segmentNormalY,
									g_segmentNormalZ
								);
							}

							sound = 0xf0;
						}
						else if (hitShape && (hitShape->m_kind & 0x200)) {
							sound = 200;
						}
						else {
							sound = 0xe5;
						}

						volume = impact;
						if (volume > 1500000) {
							volume = 1500000;
						}

						volume = MulDiv64(200, volume, 1500000);
						PlaySoundEffect(sound, volume, 0x40, 5, 0x32);
						PlayPlayerHitFeedback(g_segmentNormalX, g_segmentNormalY, g_segmentNormalZ);
					}
				}

				if (hitPlayer) {
					DamageMechsInCollision(mech, hitPlayer->m_mech);
				}
				else {
					DamageMechHittingShape(mech, hitShape);
				}
			}
		}
		else {
			if (isLocal) {
				g_collisionSoundPlayed = 0;
			}

			g_groundNormalX = g_groundNormalY = g_groundNormalZ = 0;
			mech->m_player->m_groundHeight = GetTerrainHeight(posX, posY, posZ);
			height = posY - mech->m_height - mech->m_player->m_groundHeight;
		}

		mech->m_player->m_onGround = 0;
		if (height > mech->m_height) {
			mech->m_player->m_animFlags &= ~1;
			mech->m_player->m_animFlags |= 4;
			if (isLocal && g_unk0x100a2c00) {
				g_unk0x100a2bfc = 1;
			}
		}
		else {
			mech->m_player->m_animFlags &= ~4;
			if (height <= 0) {
				mech->m_player->m_onGround = 1;
				if (height < 0) {
					if (velY < -0x56276) {
						if (mech->m_powerState == 4) {
							KnockMechOver(mech);
							return;
						}
						else {
							PlayMechLanding(mech, velY);
						}
					}

					if (velY < -0x102762 && g_difficulty->m_collisionDamage) {
						damage = FixedDiv16(velY + 0x102762, -0x204ec4) * 50;
						ApplyDamageToMech(g_localPlayerId, mech, damage, 8);
						ApplyDamageToMech(g_localPlayerId, mech, damage, 7);
					}

					mech->m_velocityY = velY = 0;
					posY = mech->m_player->m_groundHeight + mech->m_height;
				}
			}
		}

		if ((g_groundNormalX || g_groundNormalZ) && mech->m_player->m_onGround) {
			slope = FixedMul16(mech->m_player->m_headingCos, g_groundNormalZ) +
					FixedMul16(mech->m_player->m_headingSin, g_groundNormalX);
			if (slope > g_slideSlope || slope < -g_slideSlope) {
				mech->m_speed.m_value += FixedMul16(FixedMul16(g_gravity, slope), g_deltaTime);
			}
		}

		mech->m_player->m_position.m_x = posX;
		mech->m_player->m_position.m_y = posY;
		mech->m_player->m_position.m_z = posZ;
		mech->m_player->m_heading += mech->m_turnRate.m_value * g_deltaTime / 0xb5;
		mech->m_player->m_heading %= 0x1680000;
		mech->m_velocityX = velX;
		mech->m_velocityY = velY;
		mech->m_velocityZ = velZ;

		SetObjPosition(
			mech->m_player->m_obj,
			mech->m_player->m_position.m_x,
			mech->m_player->m_position.m_y,
			mech->m_player->m_position.m_z
		);
		SetObjRotation(
			mech->m_player->m_obj,
			mech->m_player->m_pitch,
			mech->m_player->m_heading,
			mech->m_player->m_roll,
			0
		);
		if (isLocal) {
			UpdateAimDistance(mech->m_player);
		}

		pitch = mech->m_torsoPitch.m_value >> 1;
		if (mech->m_pitchObj) {
			BuildAimRay(mech->m_player, &ray);
			rayLength = GetAimRange(mech->m_player);
			SetRayLength(&ray, rayLength);
			GetObjPosition(mech->m_pitchObj, &objX, &objY, &objZ);
			objX = ray.m_x1 - objX;
			objY = ray.m_y1 - objY;
			objZ = ray.m_z1 - objZ;
			angle = FixedDiv29(objY, ApproximateVectorLength(objX, objY, objZ));
			angle = -FixedAsin(angle) - pitch;
			SetObjRotation(mech->m_pitchObj, angle, 0, 0, 0);
		}

		if (mech->m_torsoObj) {
			if (mech->m_flags & 0x2000) {
				mech->m_player->m_torsoPitch = pitch;
			}
			else {
				mech->m_player->m_torsoPitch = 0;
			}

			mech->m_player->m_torsoTwist = mech->m_torsoTwist.m_value;
			mech->m_player->m_torsoRoll = 0;
			SetObjRotation(
				mech->m_torsoObj,
				mech->m_player->m_torsoPitch,
				mech->m_player->m_torsoTwist,
				mech->m_player->m_torsoRoll,
				1
			);
		}

		UpdateObj(mech->m_player->m_obj);
	}

	heading = mech->m_player->m_heading;
	mech->m_player->m_headingSin = FixedSin(heading) >> 13;
	mech->m_player->m_headingCos = FixedCos(heading) >> 13;
}

// Runs p_mech's systems for the tick: the cockpit keys of a running mech (eject, view and display
// keys, the autopilot, the throttle), its jump jets and their fuel, the turn rate, the torso's
// recentering, the MASC (which can fail), the heat, and the power state machine: starting up,
// running, shut down, destroyed (exploding until the timer runs out) and ejecting.
// Every player type's late update (PlayerType::m_lateUpdateFn).
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x100180cd
void LateUpdateMech(Mech* p_mech)
{
	MechS32 throttle;
	MechS32 isLocal;
	MechS32 cosine;
	MechS32 jumping;
	MechS32 turn;
	MechS32 speed;
	MechS32 dx;
	Mech* mech;
	MechS32 dy;
	MechS32 dz;
	MechS32 turnRate;
	MechS32 active;
	MechS32 previousDegrees;
	MechS32 degrees;
	MechS32 heading;
	MechS32 delta;
	MechChar text[40];
	MechS32 x;
	MechS32 y;
	MechS32 z;

	jumping = 0;
	speed = 0;
	turnRate = 0;
	turn = 0;
	cosine = 0;
	throttle = 0;
	active = TRUE;
	mech = p_mech;

	if (mech->m_flags & 0x200) {
		return;
	}

	isLocal = mech->m_player->m_index == g_localPlayerId;
	if (mech->m_player->m_flags & 1) {
		active = FALSE;
	}

	if (mech->m_powerState == 2) {
		UpdateWeaponFireState(mech);
	}

	if (g_debugSlot >= 0 && g_debugSection >= 0 && mech->m_player->m_index == g_localPlayerId) {
		DestroyCriticalSlot(-1, mech, g_debugSection, g_debugSlot, 0);
		g_debugSlot = -1;
	}

	UpdateAI(mech->m_player);

	if ((!g_isNetworkGame || isLocal) && mech->m_powerState == 2) {
		if (mech->m_player->m_steering->m_selfDestruct) {
			mech->m_stateTime = g_currentClock + 0x16a;
			mech->m_powerState = 7;
			if (isLocal) {
				PlayCockpitSound(4, -1);
			}

			mech->m_player->m_steering->m_selfDestruct = 0;
		}

		if (mech->m_player->m_steering->m_advanceNav) {
			CycleNavTarget(mech->m_player, 1, 0);
			mech->m_player->m_steering->m_advanceNav = 0;
		}

		if (mech->m_player->m_steering->m_previousNav) {
			CycleNavTarget(mech->m_player, -1, 0);
			mech->m_player->m_steering->m_previousNav = 0;
		}

		if (mech->m_player->m_steering->m_resetNav) {
			CycleNavTarget(mech->m_player, 0, 0);
			mech->m_player->m_steering->m_resetNav = 0;
		}

		if (mech->m_player->m_steering->m_advanceTarget) {
			CycleTarget(mech->m_player, 1, 8);
			mech->m_player->m_steering->m_advanceTarget = 0;
		}

		if (mech->m_player->m_steering->m_previousTarget) {
			CycleTarget(mech->m_player, -1, 8);
			mech->m_player->m_steering->m_previousTarget = 0;
		}

		// Clears 0x34, not 0x35.
		if (mech->m_player->m_steering->m_resetTarget) {
			CycleTarget(mech->m_player, 0, 8);
			mech->m_player->m_steering->m_previousTarget = 0;
		}

		if (mech->m_player->m_steering->m_nearestEnemy) {
			TargetNearestEnemy();
			mech->m_player->m_steering->m_nearestEnemy = 0;
		}

		if (mech->m_player->m_steering->m_targetFriendly) {
			CycleFriendlyTarget(1);
			mech->m_player->m_steering->m_targetFriendly = 0;
		}

		if (mech->m_player->m_steering->m_targetLastShot) {
			mech->m_player->m_steering->m_targetLastShot = 0;
		}

		if (mech->m_player->m_steering->m_nextObjective) {
			CycleTarget(mech->m_player, 1, 0x10008);
			mech->m_player->m_steering->m_nextObjective = 0;
		}

		if (mech->m_player->m_steering->m_advanceGamething) {
			CycleGameThingTarget(1);
			mech->m_player->m_steering->m_advanceGamething = 0;
		}

		if (mech->m_player->m_steering->m_previousGamething) {
			CycleGameThingTarget(-1);
			mech->m_player->m_steering->m_previousGamething = 0;
		}

		// Clears 0x3d, not 0x3e.
		if (mech->m_player->m_steering->m_resetGamething) {
			CycleGameThingTarget(0);
			mech->m_player->m_steering->m_previousGamething = 0;
		}

		if (mech->m_player->m_steering->m_advanceGamepiece) {
			CycleGamePieceTarget(1);
			mech->m_player->m_steering->m_advanceGamepiece = 0;
		}

		if (mech->m_player->m_steering->m_previousGamepiece) {
			CycleGamePieceTarget(-1);
			mech->m_player->m_steering->m_previousGamepiece = 0;
		}

		// Clears 0x40, not 0x41.
		if (mech->m_player->m_steering->m_resetGamepiece) {
			CycleGamePieceTarget(0);
			mech->m_player->m_steering->m_previousGamepiece = 0;
		}

		if (mech->m_player->m_steering->m_targetReticle) {
			TargetAtReticle();
			mech->m_player->m_steering->m_targetReticle = 0;
		}

		if (!(mech->m_player->m_targetInfo.m_target & 0x1000)) {
			UpdateTarget(mech->m_player);
		}

		UpdateWeaponLock(mech);

		if ((mech->m_player->m_steering->m_legsPanDelta || g_feetToTorso) && mech->m_autopilot) {
			mech->m_player->m_steering->m_autopilot = 1;
		}

		if (mech->m_player->m_steering->m_autopilot) {
			if (!mech->m_autopilot) {
				mech->m_autopilot = 1;
				if (mech->m_player->m_index == g_localPlayerId) {
					PlayCockpitSound(0x1d, 1);
				}
			}
			else {
				mech->m_autopilot = 0;
				if (mech->m_player->m_index == g_localPlayerId) {
					PlayCockpitSound(0x1d, 2);
				}
			}
		}

		mech->m_player->m_steering->m_autopilot = 0;
		if (!mech->m_player->m_aiMode && !mech->m_autopilot) {
			mech->m_player->m_steering->m_turn = mech->m_player->m_steering->m_legsPanDelta;
		}

		if (!mech->m_player->m_aiMode) {
			RunAutopilot(mech);
		}

		mech->m_throttle.m_target = FixedMul16(mech->m_player->m_steering->m_throttle, mech->m_mobility) + 0x400;
		if (mech->m_player->m_aiMode != 2 && mech->m_throttle.m_target == 0x400 &&
			(mech->m_player->m_steering->m_turn || g_feetToTorso)) {
			mech->m_throttle.m_target = 0x480;
		}
	}

	UpdateMotion(mech->m_player);

	if (mech->m_jumpFuel == -2 && mech->m_player->m_steering->m_grantJumpJets) {
		mech->m_jumpFuel = 0x712;
		mech->m_jumpJets = 3;
		mech->m_jumpThrust = g_gravity * 3;
	}

	if (mech->m_jumpFuel >= 0) {
		if (mech->m_player->m_steering->m_jumpJetEnabled && mech->m_powerState == 2) {
			jumping = 1;
			if (!g_infiniteJumpFuel || !isLocal) {
				mech->m_jumpFuel -= g_deltaTime;
			}

			if (mech->m_jumpFuel <= 0) {
				mech->m_jumpFuel = 0;
			}
		}
		else if (mech->m_jumpFuel < 0x712) {
			mech->m_jumpFuel += g_deltaTime / 4;
		}
		else {
			mech->m_jumpFuel = 0x712;
		}
	}

	if (jumping) {
		turn = 0;
		if (mech->m_jumpFuel > 0) {
			FireJumpJetEffects(mech);
			mech->m_deltaHeat += mech->m_jumpJets * g_deltaTime * 0x180;
			mech->m_player->m_animFlags &= ~4;
			mech->m_player->m_animFlags &= ~1;
			turn = mech->m_player->m_steering->m_turn / 1024 * 90;
		}

		mech->m_turnRate.m_target = turn * 2;
	}
	else if (!(mech->m_player->m_animFlags & 4) && active) {
		speed = mech->m_speed.m_value / 10000;
		if (speed || (isLocal && mech->m_mobility > 0)) {
			if (speed > 0x50) {
				speed = 0x50;
			}

			turn = mech->m_player->m_steering->m_turn / 1024 * 90;
			cosine = FixedCos(speed << 16);
			turnRate = FixedMul29(turn, cosine);
			mech->m_turnRate.m_target = turnRate;
		}
		else {
			mech->m_turnRate.m_target = 0;
		}
	}
	else if (!mech->m_autopilot) {
		mech->m_turnRate.m_target = 0;
	}

	if (isLocal && g_feetToTorso) {
		heading = mech->m_player->m_heading;
		degrees = heading >> 16;
		if (degrees > 180) {
			heading = -(0x1680000 - heading);
			degrees = heading >> 16;
		}
		else if (degrees < -180) {
			heading += 0x1680000;
			degrees = heading >> 16;
		}

		previousDegrees = g_recenterLastHeading >> 16;
		if (previousDegrees > 180) {
			g_recenterLastHeading = -(0x1680000 - g_recenterLastHeading);
			previousDegrees = g_recenterLastHeading >> 16;
		}
		else if (previousDegrees < -180) {
			g_recenterLastHeading += 0x1680000;
			previousDegrees = g_recenterLastHeading >> 16;
		}

		if (degrees > 90 && previousDegrees < -90) {
			heading = -(0x1680000 - heading);
		}
		else if (previousDegrees > 90 && degrees < -90) {
			heading += 0x1680000;
		}

		delta = heading - g_recenterLastHeading;
		if (mech->m_torsoTwist.m_value > -0x40000 && mech->m_torsoTwist.m_value < 0x40000) {
			mech->m_turnRate.m_value = mech->m_turnRate.m_target = 0;
			mech->m_torsoTwist.m_value = mech->m_torsoTwist.m_target = 0;
			g_localSteering.m_torsoPanReset = 1;
			mech->m_player->m_steering->m_torsoPan = 0;
			g_feetToTorso = 0;
		}
		else if (mech->m_torsoTwist.m_value > 0) {
			mech->m_turnRate.m_target = 0x370000;
			if (delta) {
				mech->m_torsoTwist.m_value = mech->m_torsoTwist.m_target = mech->m_torsoTwist.m_value - delta;
			}
		}
		else {
			mech->m_turnRate.m_target = -0x370000;
			if (delta) {
				mech->m_torsoTwist.m_value = mech->m_torsoTwist.m_target = mech->m_torsoTwist.m_value - delta;
			}
		}

		if (isLocal) {
			g_recenterLastHeading = mech->m_player->m_heading;
		}
	}

	if (active) {
		throttle = mech->m_throttle.m_value - 0x400;
		if (throttle < 0x20) {
			throttle = 0;
		}

		mech->m_speed.m_target = mech->m_topSpeed * throttle;
		if (mech->m_speed.m_target && mech->m_player->m_motionState == 2) {
			mech->m_speed.m_target >>= 1;
			mech->m_speed.m_target *= -1;
		}

		if (!jumping && (mech->m_player->m_animFlags & 2)) {
			if (g_mascEngaged && isLocal && g_currentClock - g_lastMascRoll > 0xb5) {
				g_lastMascRoll = g_currentClock;
				if (RandomIntBelow(0x3c) == 12 && !g_difficulty->m_invulnerable && mech->m_powerState == 2) {
					sprintf(text, "MASC malfunction.");
					ShowInGameMessage(text, 1, 0x16a, 0x32);
					PlaySoundEffect(0xc9, 100, 0x40, 5, 0x50);
					mech->m_heat += mech->m_heat >> 2;
					g_mascEngaged = 0;
				}
			}

			if (mech->m_speed.m_target > mech->m_speed.m_value) {
				mech->m_speed.m_duration = 0x5a;
			}
			else {
				mech->m_speed.m_duration = 0x5a;
			}

			if (g_mascEngaged && mech->m_player->m_index == g_localPlayerId) {
				mech->m_speed.m_target += mech->m_speed.m_target >> 1;
			}
		}

		if (!g_feetToTorso && (isLocal || !g_isNetworkGame)) {
			mech->m_torsoTwist.m_target = mech->m_player->m_steering->m_torsoPan;
		}

		if (mech->m_torsoTwist.m_target > mech->m_maxTorsoTwist) {
			mech->m_torsoTwist.m_target = mech->m_maxTorsoTwist;
			mech->m_player->m_steering->m_torsoPan = mech->m_maxTorsoTwist;
			mech->m_player->m_steering->m_torsoPanSet = 1;
		}
		else if (mech->m_torsoTwist.m_target < -mech->m_maxTorsoTwist) {
			mech->m_torsoTwist.m_target = -mech->m_maxTorsoTwist;
			mech->m_player->m_steering->m_torsoPan = -mech->m_maxTorsoTwist;
			mech->m_player->m_steering->m_torsoPanSet = 1;
		}

		if (isLocal || !g_isNetworkGame) {
			mech->m_torsoPitch.m_target = mech->m_player->m_steering->m_torsoTilt;
		}

		mech->m_deltaHeat += MulDiv64(mech->m_throttle.m_value - 0x400, mech->m_cooling, 0x2800);
	}

	if (mech->m_powerState != 2) {
		StopMotion(mech->m_player);
		mech->m_throttle.m_target = 0x400;
		mech->m_speed.m_target = 0;
		mech->m_torsoTwist.m_target = 0;
		mech->m_turnRate.m_target = 0;
		mech->m_torsoPitch.m_target = 0;
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
		if (mech->m_player->m_index == g_localPlayerId) {
			ReleaseWeaponTriggers(mech);
		}
	}

	CalculateHeat(mech);

	switch (mech->m_powerState) {
	case 7:
		if (mech->m_stateTime < g_currentClock) {
			EjectPlayer(mech, 0);
		}
		break;
	case 0:
		mech->m_stateTime = g_currentClock + RandomIntBelow(0x16a) + 0x43e;
		mech->m_powerState = 1;
		mech->m_player->m_flags &= ~0x2000;
		dx = mech->m_player->m_position.m_x - g_eyepoint->m_x;
		dy = mech->m_player->m_position.m_y - g_eyepoint->m_y;
		dz = mech->m_player->m_position.m_z - g_eyepoint->m_z;
		if (GetPlayerSide(mech->m_player->m_index) == 1) {
			if (g_players[g_localPlayerId]->m_flags & 0x2000) {
				PlaySoundAt(dx, dy, dz, 0x104, g_inCockpitView);
			}
		}
		else {
			PlaySoundAt(dx, dy, dz, 0xf7, !g_inCockpitView);
		}
		break;
	case 1:
		if (mech->m_stateTime < g_currentClock) {
			mech->m_powerState = 2;
			mech->m_player->m_flags |= 0x2000;
			if (!g_mechPoweredUp) {
				g_mechPoweredUp = 1;
			}
		}
		break;
	case 2:
		if (mech->m_player->m_flags & 0x10) {
			mech->m_player->m_flags &= ~0x10;
			mech->m_player->m_flags |= 0x2000;
			if (GetPlayerSide(mech->m_player->m_index) == 1 && (g_players[g_localPlayerId]->m_flags & 0x2000)) {
				PlayCockpitSound(0x1a, -1);
			}
		}
		break;
	case 3:
		mech->m_player->m_flags |= 0x10;
		break;
	case 4:
		mech->m_player->m_flags &= ~0x2000;
		if (mech->m_stateTime == 0) {
			EmitWreckSmoke(mech);
			mech->m_stateTime = g_currentClock + 0x712;
			FUN_1001cdd1();
		}

		if (mech->m_stateTime > g_currentClock) {
			EmitWreckSmoke(mech);
		}
		else {
			mech->m_flags |= 0x200;
		}
		break;
	}

	if (mech->m_player->m_index == g_localPlayerId &&
		(mech->m_torsoTwist.m_value + 0x20000 < mech->m_torsoTwist.m_target ||
		 mech->m_torsoTwist.m_value - 0x20000 > mech->m_torsoTwist.m_target) &&
		g_inCockpitView) {
		x = mech->m_player->m_position.m_x - g_eyepoint->m_x;
		y = mech->m_player->m_position.m_y - g_eyepoint->m_y;
		z = mech->m_player->m_position.m_z - g_eyepoint->m_z;
		PlaySoundAt(x, y, z, 0x13c, g_inCockpitView);
	}

	if (!isLocal) {
		mech->m_player->m_flags &= ~1;
	}
}

// Handles the local mech's requests for the tick. A running mech (state 2) acts on the MASC and
// other system keys; a destroyed one (4) ends the mission once; an ejecting one (5) runs the
// ejection camera and then ends it. Except while destroyed or ejecting, a power request shuts the
// mech down, or powers up a shut-down mech that isn't overheating.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10019368
void UpdateLocalMech(Mech* p_mech)
{
	MechS32 dx;
	Mech* mech;
	MechS32 dy;
	MechS32 dz;
	MechChar text[40];

	if (p_mech) {
		mech = p_mech;
	}
	else {
		return;
	}

	switch (mech->m_powerState) {
	case 5:
		if (!g_ejectStarted) {
			mech->m_stateTime = g_currentClock + 0x389;
			g_ejectStarted++;
		}
		else if (mech->m_stateTime > g_currentClock) {
			SetViewMode(c_viewDrop);
		}
		else {
			g_localMechLost = 1;
		}
		break;
	case 4:
		if (!g_localMechDestroyed) {
			g_localMechDestroyed = 1;
			FlashZappedPalette();
			g_localMechLost = 1;
			SetViewMode(c_viewTrack);
		}
		break;
	case 2:
		if (g_manualWeaponCycle) {
			if (!mech->m_unk0xb4) {
				PlaySoundEffect(0xfd, 0x32, 0x40, 5, 0x32);
				SelectNextWeapon(mech);
			}

			g_manualWeaponCycle = 0;
		}

		if (mech->m_player->m_steering->m_weaponCycle && !mech->m_unk0xb4) {
			PlaySoundEffect(0xfd, 0x32, 0x40, 5, 0x32);
			g_manualWeaponCycle = 1;
			SelectNextWeapon(mech);
			g_manualWeaponCycle = 0;
		}

		if (mech->m_player->m_steering->m_weaponCycleGroup) {
			CycleLocalWeaponGroup();
		}

		if (g_jettisonAmmoRequested) {
			g_jettisonAmmoRequested = 0;
			JettisonAmmo(mech);
		}

		if (g_toggleMascRequested) {
			g_toggleMascRequested = 0;
			if (mech->m_flags & 0x10) {
				if (g_mascEngaged) {
					g_mascEngaged = 0;
					PlaySoundEffect(0xca, 100, 0x40, 5, 0x50);
					PlayCockpitSound(0x1f, 0);
				}
				else {
					g_mascEngaged = 1;
					PlaySoundEffect(0xcb, 100, 0x40, 5, 0x50);
					PlayCockpitSound(0x1f, 1);
				}
			}
			else {
				sprintf(text, "Not equipped with MASC.");
				ShowInGameMessage(text, 1, 0x16a, 0x32);
			}
		}
	default:
		switch (g_powerRequest) {
		case 0:
			break;
		case 1:
			if ((!(mech->m_flags & 4) || (mech->m_flags & 8)) && mech->m_powerState == 3) {
				sprintf(text, "Powering up...");
				ShowInGameMessage(text, 1, 0x16a, 0x32);
				mech->m_powerState = 0;
				mech->m_player->m_flags &= ~0x10;
				g_powerRequest = 0;
			}
			break;
		case -1:
			if (mech->m_powerState != 3) {
				PlayCockpitSound(0xd, -1);
				dx = mech->m_player->m_position.m_x - g_eyepoint->m_x;
				dy = mech->m_player->m_position.m_y - g_eyepoint->m_y;
				dz = mech->m_player->m_position.m_z - g_eyepoint->m_z;
				PlaySoundAt(dx, dy, dz, 0xf2, g_inCockpitView);
				mech->m_powerState = 3;
				g_powerRequest = 0;
			}
			break;
		}
		break;
	}
}

// FUNCTION: MW2 0x1001975a
void DrawMechCockpit(Mech* p_mech)
{
	Mech* mech;

	if (p_mech) {
		mech = p_mech;
	}
	else {
		return;
	}

	UpdateCockpit(mech);
}

// Runs ShutdownCockpitPanels for the local player's mech.
// FUNCTION: MW2 0x1001978e
void ShutdownMech(Mech* p_mech)
{
	Mech* mech;

	mech = p_mech;
	if (!p_mech) {
		return;
	}

	if (mech->m_player->m_index == g_localPlayerId) {
		ShutdownCockpitPanels();
	}
}

// Allocates p_player's mech and sets it up.
// Stack-slot permutation of buffer, i, size and mech.
// FUNCTION: MW2 0x100197ca
MechS32 CreateMech(MechS32 p_index, Player* p_player)
{
	void* buffer = NULL;
	MechS32 i;
	MechS32 size;
	Mech* mech = NULL;

	p_player->m_mech = NULL;
	size = GetMechAllocSize();
	buffer = StaticPoolAlloc(size, g_staticPoolTags[1]);
	if (!buffer) {
		return FALSE;
	}

	mech = buffer;
	mech->m_player = p_player;
	for (i = 0; i < 8; i++) {
		mech->m_objects[i] = NULL;
	}

	p_player->m_mech = mech;
	p_player->m_mechSize = 0x10e;
	InitMechArrays(mech);
	return TRUE;
}

// Lays out p_mech's allocation (its ten weapons, eight sections and 25 ammunition bins after it)
// and empties the weapons and the bins.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10019881
void InitMechArrays(struct Mech* p_mech)
{
	AmmoBin* bin;
	WeaponSlot* slot;
	AmmoBin* bins = NULL;
	MechS32 i;
	WeaponSlot* weapons = NULL;
	MechSection* sections = NULL;

	weapons = (WeaponSlot*) (p_mech + 1);
	sections = (MechSection*) (weapons + 10);
	bins = (AmmoBin*) (sections + 8);
	p_mech->m_weapons = weapons;
	p_mech->m_sections = sections;
	p_mech->m_ammoBins = bins;
	slot = p_mech->m_weapons;
	for (i = 0; i < 10; i++) {
		slot->m_status = -1;
		slot->m_type = -1;
		slot->m_state = c_weaponEmpty;
		slot->m_time = 0;
		slot->m_unk0x14 = 0;
		slot->m_ammo = -1;
		slot->m_group = 0;
		slot->m_target = -1;
		slot->m_targetKind = 0;
		slot->m_volley = 0;
		slot->m_hardpoint = -1;
		slot->m_slotId = 0;
		slot->m_binCount = 0;
		slot->m_index = 0;
		slot++;
	}

	bin = p_mech->m_ammoBins;
	for (i = 0; i < 25; i++) {
		bin->m_type = -1;
		bin->m_shots = 0;
		bin->m_weapon = -1;
		bin->m_id = 0;
		bin->m_unk0x08 = 0;
		bin->m_damage = 0;
		bin->m_shotHeat = 0;
		bin->m_heat = 0;
		bin++;
	}
}

// Returns the size of a mech's allocation: the mech, its ten weapons and eight sections, and 500
// bytes more.
// FUNCTION: MW2 0x10019a0a
MechS32 GetMechAllocSize(void)
{
	MechS32 size;

	size = sizeof(Mech);
	size += 10 * sizeof(WeaponSlot);
	size += 8 * sizeof(MechSection);
	size += 500;
	return size;
}

// Nothing calls it.
// FUNCTION: MW2 0x10019a3c
MechS32 GetLastSelectedWeapon(Player* p_player)
{
	Mech* mech;

	mech = p_player->m_mech;
	return mech->m_lastSelectedWeapon;
}

// Selects weapon p_weapon of p_player's mech.
// FUNCTION: MW2 0x10019a61
void SetSelectedWeapon(Player* p_player, MechS32 p_weapon)
{
	Mech* mech;

	mech = p_player->m_mech;
	mech->m_selectedWeapon = p_weapon;
}

// Nothing calls it.
// FUNCTION: MW2 0x10019a84
MechS32 GetMechHeight(Player* p_player)
{
	Mech* mech;

	mech = p_player->m_mech;
	return mech->m_height;
}
