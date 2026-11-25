#include "fadepal.h"

#include "animation.h"
#include "camerashake.h"
#include "clock.h"
#include "debris.h"
#include "depthsort.h"
#include "displaybackend.h"
#include "eyepoint.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "fixedsqrt.h"
#include "loadres.h"
#include "mech.h"
#include "mw2prj.h"
#include "object.h"
#include "palette.h"
#include "players.h"
#include "polydraw.h"
#include "random.h"
#include "refreshmode.h"
#include "render.h"
#include "rendersettings.h"
#include "shapelists.h"
#include "shots.h"
#include "simmain.h"
#include "soundfx.h"
#include "types.h"
#include "view.h"

// The launch sound of the local player's weapon, in the cockpit (p_shotType is unused).
void PlayWeaponLaunchSound(uint32_t p_shotType, int32_t p_sound, uint32_t p_pan)
{
	if (p_sound > 0) {
		PlayDelayedSound(0, 0, p_sound, 0x32, p_pan, 0x32);
	}
}

// Renders the scene into pane p_target from the camera p_view with a field of view of
// p_fovX (only the objects under p_object, if set), then restores the eyepoint, its field of view
// and the pending palette.
void RenderViewToPane(uint32_t p_target, int32_t p_fovX, int32_t* p_view, struct SceneObject* p_object)
{
	int32_t fovX;
	int32_t palette;
	int32_t view[7];

	fovX = g_eyepoint->m_fovX;
	palette = g_palettePending;
	SaveView(g_eyepoint, view);
	SelectPane(p_target);
	g_eyepoint->m_fovX = p_fovX;
	p_view[6] = 1;
	RestoreView(g_eyepoint, p_view);
	UpdateProjection(g_eyepoint);
	UpdateViewMatrix(g_eyepoint);
	SelectEyepoint(g_eyepoint);
	if (g_renderSettings.m_drawSky || g_renderSettings.m_drawGround) {
		DrawSkyAndGround(g_eyepoint);
	}

	if (p_object) {
		DrawObjTreeShapes(p_object);
	}
	else {
		DrawShapeList(g_sceneShapes);
	}

	g_palettePending = palette;
	ResetPane();
	g_eyepoint->m_fovX = fovX;
	g_projectionDirty = 1;
	RestoreView(g_eyepoint, view);
	UpdateProjection(g_eyepoint);
	UpdateViewMatrix(g_eyepoint);
	SelectEyepoint(g_eyepoint);
	g_projectionDirty = 0;
}

// Flashes palette slot 0x11 (the ZAPPED palette, solid red) over two seconds (0x16a clock ticks).
void FlashZappedPalette(void)
{
	StartPaletteFade(0x11, 0x16a, 1);
}

// Flashes palette slot 0x11 (ZAPPED) over p_level (0-15) fifteenths of two seconds.
void FlashZappedPaletteLevel(uint32_t p_level)
{
	int32_t duration;

	if (p_level > 15) {
		p_level = 0x10000;
	}
	else {
		p_level = FixedDiv16(p_level, 15);
	}

	duration = FixedMul16(0x16a, p_level);
	StartPaletteFade(0x11, duration, 1);
}

// Fades (blocking, over 60 steps) to palette slot 0x10, or 0x11 if p_alternate is set, then applies
// that slot.
void FadeToEndPalette(int32_t p_alternate)
{
	int32_t slot;
	PaletteColor* palette;

	if (p_alternate) {
		slot = 0x11;
	}
	else {
		slot = 0x10;
	}

	palette = LoadCachedResource(g_mw2PrjHandle, g_paletteResourceIds[slot], g_resourceTypeTags[c_resTagPal], 0);
	if (palette) {
		g_currentDisplayBackend->m_blendPalettes(palette, 0x3c);
		UnlockCachedResource(g_paletteResourceIds[slot], g_resourceTypeTags[c_resTagPal]);
		ApplyPaletteResource(slot);
	}
}

// Sets off the smoke of a wrecked mech, or now and then a spark while m_stateTime is set.
void EmitWreckSmoke(Mech* p_mech)
{
	int32_t x;
	int32_t y;
	int32_t z;

	x = p_mech->m_player->m_position.m_x;
	y = p_mech->m_player->m_position.m_y;
	z = p_mech->m_player->m_position.m_z;
	if (!p_mech->m_stateTime) {
		SpawnEffect(p_mech->m_player->m_killer, 0xd, x, y, z, x, y, z);
	}
	else if (CrossedIdealFrame() && RandomIntBelow(100) <= 20) {
		x += RandomNormal() / 2;
		z += RandomNormal() / 2;
		if (RandomIntBelow(100) < 0x3c) {
			SpawnEffect(p_mech->m_player->m_killer, 3, x, y, z, x, y, z);
		}
		else {
			SpawnEffect(p_mech->m_player->m_killer, 0x10b, x, y, z, x, y, z);
		}
	}
}

// Breaks a destroyed mech's model into debris.
void BreakUpMech(Mech* p_mech)
{
	SceneObject* obj;
	SceneObject* upper;
	SceneObject* lower;
	int32_t upperIndex;
	int32_t lowerIndex;
	int32_t objIndex;

	ShowObjTree(p_mech->m_player->m_obj);
	obj = p_mech->m_player->m_obj;
	upper = p_mech->m_pitchObj;
	lower = p_mech->m_torsoObj;
	upperIndex = AddDebrisPiece(upper);
	ThrowDebrisPiece(upperIndex);
	lowerIndex = AddDebrisPiece(lower);
	ThrowDebrisPiece(lowerIndex);
	objIndex = AddDebrisPiece(obj);
	ThrowDebrisPiece(objIndex);
}

// Sets off the flames of the jump jets and plays their sound.
void FireJumpJetEffects(Mech* p_mech)
{
	int32_t z;
	int32_t y;
	int32_t x;
	Player* player;
	int32_t jet;
	int32_t fired;

	fired = FALSE;
	player = p_mech->m_player;
	jet = 6;
	if (p_mech->m_objects[jet]) {
		fired = TRUE;
		player->m_firingObj = p_mech->m_objects[jet];
		SpawnLaunchEffect(0x19, p_mech->m_player);
		GetObjPosition(player->m_firingObj, &x, &y, &z);
	}

	jet = 7;
	if (p_mech->m_objects[jet]) {
		fired = TRUE;
		player->m_firingObj = p_mech->m_objects[jet];
		SpawnLaunchEffect(0x19, p_mech->m_player);
		GetObjPosition(player->m_firingObj, &x, &y, &z);
	}

	if (fired) {
		x -= g_eyepoint->m_x;
		y -= g_eyepoint->m_y;
		z -= g_eyepoint->m_z;
		// The original game used > for the previous-fuel check. Changing it to >= makes it reliable.
		if (p_mech->m_player->m_index == g_localPlayerId && p_mech->m_jumpFuel < 0x1c4 &&
			p_mech->m_jumpFuel + g_deltaTime >= 0x1c4) {
			// "Low fuel" warning beeps
			PlaySoundEffect(0xde, 100, 0x40, 5, 0x50);
		}
		else {
			if (CrossedIdealFrame()) {
				PlaySoundAt(x, y, z, 0xdf, g_inCockpitView);
			}
		}
	}
}

// Plays a mech's landing: the thud, and the camera shake for the local player.
void PlayMechLanding(Mech* p_mech, int32_t p_speed)
{
	int32_t sound;

	if (p_mech->m_player->m_index == g_localPlayerId) {
		PlayPlayerHitFeedback(0, -p_speed, 0);
	}

	if (p_speed >= -0x102762) {
		sound = 0xe6;
	}
	else {
		sound = 0xe5;
	}

	PlaySoundAt(
		p_mech->m_player->m_position.m_x - g_eyepoint->m_x,
		p_mech->m_player->m_position.m_y - p_mech->m_height - g_eyepoint->m_y,
		p_mech->m_player->m_position.m_z - g_eyepoint->m_z,
		sound,
		g_inCockpitView
	);
}

// Shakes the camera away from a hit's direction, unless a shake is already playing.
void PlayPlayerHitFeedback(int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t off10;
	int32_t off0c;
	int32_t off14;

	if (IsCameraShaking()) {
		return;
	}

	NormalizeVectorGuarded(&p_x, &p_y, &p_z);
	if (p_x == 0 && p_z == 0) {
		off10 = p_y * 30;
	}
	else {
		off10 = (p_y - p_z) * 5;
	}

	off0c = 0;
	off14 = 0;
	p_x = FixedMul16(p_x, -25);
	p_y = FixedMul16(p_y, -25);
	p_z = FixedMul16(p_z, -25);
	ClearCameraShakeKeys();
	AddCameraShakeKey(p_x, p_y, p_z, off10, off0c, off14, 0.2);
	AddCameraShakeKey(-(p_x >> 1), -(p_y >> 1), -(p_z >> 1), -(off10 >> 1), -(off0c >> 1), -(off14 >> 1), 0.5);
	AddCameraShakeKey(0, 0, 0, 0, 0, 0, 0.2);
	StartCameraShake();
}
