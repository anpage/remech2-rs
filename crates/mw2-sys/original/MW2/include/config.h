#ifndef CONFIG_H
#define CONFIG_H

#include "cockpitpanel.h"
#include "decomp.h"
#include "recttransition.h"
#include "soundconfig.h"
#include "targeting.h"
#include "types.h"

struct Mech;
struct Point;
struct ResourceRef;

#pragma pack(1)
// The difficulty settings, read from a .cfg file as one block.
// SIZE 0x17
typedef struct DifficultyCfg {
	undefined m_unlimitedAmmo;   // 0x00 — the "cia" cheat; cleared in network games with more than one player
	undefined m_invulnerable;    // 0x01 — the "blorb" cheat; cleared in network games
	undefined m_splashDamage;    // 0x02 — splash damage hurts mechs; set in network games with more than one player
	undefined m_collisionDamage; // 0x03 — collisions hurt mechs; set in network games with more than one player
	undefined m_heatTracking;    // 0x04 — fires heat mechs nearby; set in network games with more than one player
	undefined m_enemySkill;      // 0x05 — 0 easy, 1 medium, 2 hard (MW2SHELL); 2 in network games
	undefined m_unk0x06[2];      // 0x06
	undefined m_regenerate;      // 0x08 — after the mission ends, the local player can regenerate
	undefined
		m_radar; // 0x09 — the radar and auto targeting (NETMECHW's "Radar + Auto Targeting"); set outside network games
	undefined m_teamGame;     // 0x0a
	undefined4 m_timeOfDay;   // 0x0b — the time of day phase a network game starts at; cleared outside network games
	undefined4 m_gravity;     // 0x0f — overrides the planet's gravity when set; cleared outside network games
	undefined4 m_temperature; // 0x13 — overrides the planet's temperature when set; cleared outside network games
} DifficultyCfg;
#pragma pack()

// The functions and globals of config.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_hudLayoutValues[3];
	extern MechS32 g_torsoTwistDegrees;
	extern MechS32 g_headingDegrees;
	extern CockpitPanel* g_cockpitPanels[c_panelCount];
	extern MechS32 g_cockpitPanelEnabled[c_panelCount];
	extern MechS32 g_cockpitPowerState;
	extern struct PANE g_cockpitPanelPanes[c_panelCount];
	extern MechS32 g_punchInAutoHeadingRequested;
	extern MechChar g_gameDir[256];
	extern MechS32 g_hitFadePending;
	extern Point g_cockpitPanelTextOrigins[c_panelCount];
	extern RectTransitionState g_targetTransitionState;
	extern RectTransitionState g_mechViewTransitionState;
	extern PANE g_targetTransitionFirst;
	extern PANE g_targetTransitionSecond;
	extern PANE g_targetTransitionRect;
	extern RectTransitionDef g_targetTransitionDef;
	extern RectTransition g_targetTransition;
	extern PANE g_mechViewTransitionFirst;
	extern PANE g_mechViewTransitionSecond;
	extern PANE g_mechViewTransitionRect;
	extern RectTransitionDef g_mechViewTransitionDef;
	extern RectTransition g_mechViewTransition;
	extern RectTransition* g_cockpitPanelTransitions[c_panelCount];
	extern undefined4 g_cockpitPanelLightUpTimes[c_panelCount];
	extern MechS32 g_lastWarningPowerState;
	extern MechS32 g_lockedTonePlayed;
	extern MechS32 g_lockingTonePlayed;
	extern MechS32 g_hitFadeCount;
	extern MechS32 g_screenshotCount;
	extern MechChar g_gamePath[0x50];

	MechS32 LoadFile(MechChar* p_path, MechS32* p_size, void** p_data, MechU32* p_poolTag);
	void SaveScreenshot(void);
	MechS32 LoadDifficultyCfg(MechChar* p_name, DifficultyCfg** p_cfg);
	MechS32 WriteCareerRecordFile(MechChar* p_name, void* p_data);
	MechS32 LoadSndCfg(MechChar* p_name, SoundConfig** p_cfg);
	MechS32 SaveSndCfg(MechChar* p_name, SoundConfig* p_cfg);
	MechChar* BuildGamePath(MechChar* p_name);
	void LayoutWeaponPanels(struct Mech* p_mech);
	void ScaleCockpitLayout(void);
	void InitCockpitPanels(void);
	void ResetCockpitPanels(void);
	void UpdateCockpit(struct Mech* p_mech);
	void ShutdownCockpitPanels(void);
	void DamageCockpitPanels(struct Mech* p_mech, MechS32 p_heavy);
	void PlayCockpitWarnings(struct Mech* p_mech);
	void DrawPanelAnim(struct PANE* p_target, MechS32 p_index, MechS32 p_x, MechS32 p_y);
	MechS32 LoadMgdFile(
		struct ResourceRef* p_ref,
		MechS32* p_height,
		MechS32* p_cockpitHeight,
		MechS32* p_unk0x0c,
		MechS32* p_unk0x10,
		MechS32* p_unk0x14,
		MechS32* p_maxTorsoTwist,
		MechS32* p_radius
	);
	MechS32 LoadReels(struct ResourceRef* p_ref);
	MechS32 LoadHudFile(struct ResourceRef* p_ref);
	MechS32 LoadCptFile(struct ResourceRef* p_ref, struct PANE* p_gauges, struct PANE* p_panels, struct Point* p_point);
	void PreloadCockpitSounds(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H
