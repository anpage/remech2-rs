#include "debris.h"

#include "approxlen.h"
#include "clock.h"
#include "collision.h"
#include "debrischunk.h"
#include "debrispiece.h"
#include "decomp.h"
#include "environment.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "inradius.h"
#include "integrate.h"
#include "mechclass.h"
#include "object.h"
#include "random.h"
#include "shape.h"
#include "shots.h"
#include "types.h"

DECOMP_SIZE_ASSERT(DebrisPiece, 0x24)
DECOMP_SIZE_ASSERT(DebrisChunk, 0x14)

// Wreckage: scene objects knocked off a model fly as debris pieces, bounce on the terrain and
// come to rest; the chunks among them can be shot to pieces and time out.

// GLOBAL: MW2 0x100a1150
MechS32 g_debrisCount = 0;

// GLOBAL: MW2 0x100a1158
DebrisChunk g_emptyDebrisChunk = {0};

// GLOBAL: MW2 0x10179ec0
DebrisPiece g_debrisPieces[0x80];

// GLOBAL: MW2 0x1017b0c0
DebrisChunk g_debrisChunks[0x80];

// FUNCTION: MW2 0x100040b0
MechS32 IsDebrisFull(void)
{
	MechS32 i;

	for (i = 0; i < 0x80 && g_debrisPieces[i].m_obj; i++) {
	}

	return i == 0x80;
}

// Stack-slot permutation: i and piece.
// FUNCTION: MW2 0x10004111
MechS32 AddDebrisPiece(SceneObject* p_obj, MechS32 p_unk0x00)
{
	MechS32 i;
	DebrisPiece* piece;

	if (p_obj == NULL) {
		return -1;
	}

	for (i = 0; i < 0x80 && g_debrisPieces[i].m_obj != p_obj; i++) {
	}

	if (i == 0x80) {
		for (i = 0; i < 0x80 && g_debrisPieces[i].m_obj; i++) {
		}
	}

	if (i == 0x80) {
		return -1;
	}

	ResetDebrisPiece(i);
	g_debrisCount++;
	DetachObj(p_obj);

	piece = &g_debrisPieces[i];
	piece->m_unk0x00 = p_unk0x00;
	piece->m_obj = p_obj;
	piece->m_acceleration = -g_gravity;
	return i;
}

// Throws the piece off in a random direction, spinning.
// FUNCTION: MW2 0x10004218
void ThrowDebrisPiece(MechS32 p_index)
{
	DebrisPiece* piece;

	piece = &g_debrisPieces[p_index];
	if (!piece->m_obj) {
		return;
	}

	piece->m_velocityX = FixedMul16((RandomNormal() << 16) / ((g_gravityScale << 10) >> 16), 0x57e98);
	piece->m_velocityZ = FixedMul16((RandomNormal() << 16) / ((g_gravityScale << 10) >> 16), 0x57e98);
	piece->m_velocityY = FixedMul16(((RandomNormal() + 0x400) << 16) / ((g_gravityScale << 10) >> 16), 0x57e98);
	piece->m_spinX = RandomNormal() * 0x7e98 / 0x400;
	piece->m_spinY = RandomNormal() * 0x7e98 / 0x400;
	piece->m_spinZ = RandomNormal() * 0x7e98 / 0x400;
}

// Blows p_obj off its model as a chunk of debris; p_callback gets it when it's gone.
// Stack-slot permutation: i and chunk.
// FUNCTION: MW2 0x10004356
void BlowOffChunk(SceneObject* p_obj, ObjectCallback p_callback, MechU32 p_unk0x16)
{
	MechS32 i;
	MechS32 destroy;
	MechS32 piece;
	DebrisChunk* chunk;

	if (!p_obj || !p_obj->m_shape) {
		return;
	}

	destroy = FALSE;
	if ((p_obj->m_shape->m_kind & 0xf0) == 0x70 || (p_unk0x16 && p_obj->m_shape->m_partId == 0)) {
		SetShapeKind(p_obj->m_shape, 0x50);
		destroy = TRUE;
	}

	for (i = 0; i < 0x80 && g_debrisChunks[i].m_active; i++) {
	}

	if (i == 0x80 || destroy) {
		DetachObj(p_obj);
		DisposeDebris(p_obj, p_callback);
		return;
	}
	else {
		chunk = &g_debrisChunks[i];
	}

	piece = AddDebrisPiece(p_obj, 2);
	if (piece >= 0) {
		chunk->m_active = TRUE;
		chunk->m_obj = p_obj;
		chunk->m_callback = p_callback;
		chunk->m_startTime = g_currentClock;
		chunk->m_health = 0x100000;
		ClearObjTreeKind(p_obj, 0x300);
		SetObjTreeKind(p_obj, 0x50);
		SetObjTreeOwner(p_obj, i);
		ThrowDebrisPiece(piece);
	}
	else {
		DisposeDebris(p_obj, p_callback);
	}
}

// Stack-slot permutation: child and sibling.
// FUNCTION: MW2 0x100044f3
void BlowOffObjTree(SceneObject* p_obj, ObjectCallback p_callback, MechU32 p_unk0x16)
{
	SceneObject* child;
	SceneObject* sibling;

	if (!p_obj) {
		return;
	}

	child = GetObjFirstChild(p_obj);
	if (child) {
		BlowOffObjTree(child, p_callback, p_unk0x16);
	}

	sibling = GetObjNextSibling(p_obj);
	if (sibling) {
		BlowOffObjTree(sibling, p_callback, p_unk0x16);
	}

	BlowOffChunk(p_obj, p_callback, p_unk0x16);
}

// FUNCTION: MW2 0x1000457e
void DisposeDebris(SceneObject* p_obj, ObjectCallback p_callback)
{
	MechS32 index;

	if (p_obj) {
		HideDebrisObj(p_obj);
		if (p_callback) {
			p_callback(p_obj);
		}

		index = FindDebrisPiece(p_obj);
		if (index != -1) {
			ResetDebrisPiece(index);
		}
	}
}

// FUNCTION: MW2 0x100045db
void UpdateDebris(void)
{
	MechS32 i;

	for (i = 0; i < 0x80; i++) {
		UpdateDebrisPiece(i);
	}

	for (i = 0; i < 0x80; i++) {
		if (g_debrisChunks[i].m_active == TRUE && g_currentClock - g_debrisChunks[i].m_startTime > 0xe24 &&
			!g_localMechLost) {
			DisposeDebris(g_debrisChunks[i].m_obj, g_debrisChunks[i].m_callback);
			g_debrisChunks[i] = g_emptyDebrisChunk;
		}
	}
}

// Blows the chunk up.
// FUNCTION: MW2 0x100046b2
void ExplodeChunk(MechS32 p_index)
{
	DebrisChunk* chunk;
	SceneObject* obj;
	MechS32 x;
	MechS32 y;
	MechS32 z;
	MechS32 piece;

	chunk = &g_debrisChunks[p_index];
	obj = chunk->m_obj;
	if (obj) {
		GetShapeBounds(GetObjShape(obj), &x, &y, &z);
		SpawnEffect(-2, 7, x, y, z, x, y, z);
		HideDebrisObj(obj);
		if (chunk->m_callback) {
			chunk->m_callback(obj);
		}

		piece = FindDebrisPiece(obj);
		if (piece != -1) {
			ResetDebrisPiece(piece);
		}

		*chunk = g_emptyDebrisChunk;
	}
}

// FUNCTION: MW2 0x10004783
void DamageChunk(MechS32 p_index, MechS32 p_damage)
{
	g_debrisChunks[p_index].m_health -= p_damage;
	if (g_debrisChunks[p_index].m_health < 0) {
		ExplodeChunk(p_index);
	}
}

// Stack-slot permutation of the locals, and the operand order of newY <= ground.
// FUNCTION: MW2 0x100047c2
void UpdateDebrisPiece(MechS32 p_index)
{
	MechS32 velocityY;
	MechS32 rising;
	MechS32 landed;
	DebrisPiece* piece;
	MechS32 ground;
	MechS32 radius;
	MechS32 z;
	MechS32 spinZ;
	MechS32 y;
	MechS32 spinY;
	MechS32 x;
	MechS32 spinX;
	MechS32 dz;
	MechS32 newY;
	MechS32 dx;
	MechS32 dy;

	landed = FALSE;
	piece = &g_debrisPieces[p_index];
	if (!piece->m_obj) {
		return;
	}

	radius = GetShapeBounds(GetObjShape(piece->m_obj), &x, &y, &z);
	y = y - (radius >> 1);
	newY = y;
	velocityY = piece->m_velocityY;
	rising = velocityY > 0;
	IntegrateMidpoint(&newY, &velocityY, piece->m_acceleration, g_deltaTime);

	if (velocityY <= 0 && newY <= (ground = GetTerrainHeight(x, y, z))) {
		if (rising) {
			HideDebrisObj(piece->m_obj);
			landed = TRUE;
		}

		if (velocityY > -0x8d6f) {
			landed = TRUE;
		}

		newY = ground;
		velocityY = -(velocityY >> 2);
		if (RandomIntBelow(2)) {
			velocityY >>= 1;
			piece->m_velocityX = -(piece->m_velocityX >> 1);
			piece->m_velocityZ = -(piece->m_velocityZ >> 1);
		}

		piece->m_spinX = -(piece->m_spinX >> 1);
		piece->m_spinY = -(piece->m_spinY >> 1);
		piece->m_spinZ = -(piece->m_spinZ >> 1);
	}

	piece->m_velocityY = velocityY;
	dx = FixedMul16(piece->m_velocityX, g_deltaTime);
	dy = newY - y;
	dz = FixedMul16(piece->m_velocityZ, g_deltaTime);
	spinX = FixedMul16(piece->m_spinX, g_deltaTime << 16);
	spinY = FixedMul16(piece->m_spinY, g_deltaTime << 16);
	spinZ = FixedMul16(piece->m_spinZ, g_deltaTime << 16);
	MoveObj(piece->m_obj, dx, dy, dz);
	RotateObj(piece->m_obj, spinX, spinY, spinZ, 0);
	UpdateObj(piece->m_obj);

	if (landed) {
		g_debrisCount--;
		ResetDebrisPiece(p_index);
	}
}

// Pushes the piece by (p_x, p_y, p_z) on top of a random throw.
// Stack-slot permutation: length, speed and piece; and the operand order of length < speed.
// FUNCTION: MW2 0x10004a45
void PushDebrisPiece(MechS32 p_index, MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	MechS32 length;
	MechS32 speed;
	DebrisPiece* piece;

	piece = &g_debrisPieces[p_index];
	if (!piece->m_obj) {
		return;
	}

	length = ApproximateVectorLength(p_x, p_y, p_z);
	if (!length) {
		return;
	}

	ThrowDebrisPiece(p_index);
	speed = ApproximateVectorLength(piece->m_velocityX, piece->m_velocityY, piece->m_velocityZ);
	if (length < speed) {
		speed = FixedDiv16(length * 2, speed);
		piece->m_velocityX = FixedMul16(piece->m_velocityX, speed);
		piece->m_velocityY = FixedMul16(piece->m_velocityY, speed);
		piece->m_velocityZ = FixedMul16(piece->m_velocityZ, speed);
	}

	piece->m_velocityX += p_x;
	piece->m_velocityY += p_y;
	piece->m_velocityZ += p_z;
}

// The only diff is the indirect call's displacement (g_debrisChunks[0].m_callback), which
// reccmp leaves unmapped.
// FUNCTION: MW2 0x10004b4f
void ZeroChunx(void)
{
	MechS32 i;
	SceneObject* obj;

	for (i = 0; i < 0x80; i++) {
		obj = g_debrisChunks[i].m_obj;
		if (obj && g_debrisChunks[i].m_callback) {
			g_debrisChunks[i].m_callback(obj);
		}

		g_debrisChunks[i] = g_emptyDebrisChunk;
	}

	for (i = 0; i < 0x80; i++) {
		ResetDebrisPiece(i);
	}
}

// FUNCTION: MW2 0x10004c06
void ResetDebrisPiece(MechS32 p_index)
{
	DebrisPiece* piece;

	piece = &g_debrisPieces[p_index];
	piece->m_unk0x00 = 0;
	piece->m_obj = NULL;
	piece->m_velocityX = piece->m_velocityY = piece->m_velocityZ = 0;
	piece->m_spinX = piece->m_spinY = piece->m_spinZ = 0;
	piece->m_acceleration = 0;
}

// Stack-slot permutation: index and i.
// FUNCTION: MW2 0x10004c86
MechS32 FindDebrisPiece(SceneObject* p_obj)
{
	MechS32 index;
	MechS32 i;

	index = -1;
	for (i = 0; i < 0x80; i++) {
		if (g_debrisPieces[i].m_obj == p_obj) {
			index = i;
			break;
		}
	}

	return index;
}

// Damages the chunks within p_radius of (p_x, p_y, p_z) by p_damage per second.
// Stack-slot permutation of the locals, and the operand order of radius + p_radius.
// FUNCTION: MW2 0x10004ce5
void DamageChunksInRadius(MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_radius, MechS32 p_damage)
{
	DebrisChunk* chunk;
	MechS32 x;
	MechS32 y;
	MechS32 z;
	MechS32 radius;
	MechS32 dx;
	MechS32 dy;
	MechS32 dz;
	MechS32 reach;
	MechS32 i;

	i = 0x80;
	while (i--) {
		chunk = &g_debrisChunks[i];
		if (!chunk->m_active || !chunk->m_obj) {
			continue;
		}

		radius = GetShapeBounds(GetObjShape(chunk->m_obj), &x, &y, &z);
		dx = x - p_x;
		dy = y - p_y;
		dz = z - p_z;
		reach = radius + p_radius;
		if (IsWithinRadius(dx, dy, dz, reach)) {
			DamageChunk(i, FixedMul16(p_damage, g_deltaTime));
		}
	}
}

// FUNCTION: MW2 0x10004dcb
void RemoveChunk(SceneObject* p_obj, ObjectCallback p_callback)
{
	MechS32 i;

	if (p_obj == NULL) {
		return;
	}

	for (i = 0; i < 0x80; i++) {
		if (g_debrisChunks[i].m_obj == p_obj) {
			g_debrisChunks[i] = g_emptyDebrisChunk;
			break;
		}
	}

	DisposeDebris(p_obj, p_callback);
}

// FUNCTION: MW2 0x10004e4d
void HideDebrisObj(SceneObject* p_obj)
{
	if (p_obj) {
		HideObjTree(p_obj);
		DisableObjTreeCollision(p_obj);
		UpdateObj(p_obj);
	}
}
