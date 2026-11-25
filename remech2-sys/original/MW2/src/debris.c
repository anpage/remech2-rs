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

// Wreckage: scene objects knocked off a model fly as debris pieces, bounce on the terrain and
// come to rest; the chunks among them can be shot to pieces and time out.

int32_t g_debrisCount = 0;

DebrisChunk g_emptyDebrisChunk = {0};

DebrisPiece g_debrisPieces[0x80];

DebrisChunk g_debrisChunks[0x80];

int32_t IsDebrisFull(void)
{
	int32_t i;

	for (i = 0; i < 0x80 && g_debrisPieces[i].m_obj; i++) {
	}

	return i == 0x80;
}

int32_t AddDebrisPiece(SceneObject* p_obj)
{
	int32_t i;
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
	piece->m_obj = p_obj;
	piece->m_acceleration = -g_gravity;
	return i;
}

// Throws the piece off in a random direction, spinning.
void ThrowDebrisPiece(int32_t p_index)
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
void BlowOffChunk(SceneObject* p_obj, ObjectCallback p_callback, uint32_t p_unk0x16)
{
	int32_t i;
	int32_t destroy;
	int32_t piece;
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

	piece = AddDebrisPiece(p_obj);
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

void BlowOffObjTree(SceneObject* p_obj, ObjectCallback p_callback, uint32_t p_unk0x16)
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

void DisposeDebris(SceneObject* p_obj, ObjectCallback p_callback)
{
	int32_t index;

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

void UpdateDebris(void)
{
	int32_t i;

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
void ExplodeChunk(int32_t p_index)
{
	DebrisChunk* chunk;
	SceneObject* obj;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t piece;

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

void DamageChunk(int32_t p_index, int32_t p_damage)
{
	g_debrisChunks[p_index].m_health -= p_damage;
	if (g_debrisChunks[p_index].m_health < 0) {
		ExplodeChunk(p_index);
	}
}

void UpdateDebrisPiece(int32_t p_index)
{
	int32_t velocityY;
	int32_t rising;
	int32_t landed;
	DebrisPiece* piece;
	int32_t ground;
	int32_t radius;
	int32_t z;
	int32_t spinZ;
	int32_t y;
	int32_t spinY;
	int32_t x;
	int32_t spinX;
	int32_t dz;
	int32_t newY;
	int32_t dx;
	int32_t dy;

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
void PushDebrisPiece(int32_t p_index, int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t length;
	int32_t speed;
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

void ZeroChunx(void)
{
	int32_t i;
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

void ResetDebrisPiece(int32_t p_index)
{
	DebrisPiece* piece;

	piece = &g_debrisPieces[p_index];
	piece->m_obj = NULL;
	piece->m_velocityX = piece->m_velocityY = piece->m_velocityZ = 0;
	piece->m_spinX = piece->m_spinY = piece->m_spinZ = 0;
	piece->m_acceleration = 0;
}

int32_t FindDebrisPiece(SceneObject* p_obj)
{
	int32_t index;
	int32_t i;

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
void DamageChunksInRadius(int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_radius, int32_t p_damage)
{
	DebrisChunk* chunk;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t radius;
	int32_t dx;
	int32_t dy;
	int32_t dz;
	int32_t reach;
	int32_t i;

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

void RemoveChunk(SceneObject* p_obj, ObjectCallback p_callback)
{
	int32_t i;

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

void HideDebrisObj(SceneObject* p_obj)
{
	if (p_obj) {
		HideObjTree(p_obj);
		DisableObjTreeCollision(p_obj);
		UpdateObj(p_obj);
	}
}
