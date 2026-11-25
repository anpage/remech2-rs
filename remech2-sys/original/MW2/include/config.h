#ifndef CONFIG_H
#define CONFIG_H

#include "cockpitpanel.h"
#include "recttransition.h"
#include "soundconfig.h"
#include "targeting.h"
#include "types.h"

struct Mech;
struct Point;
struct ResourceRef;

#pragma pack(1)
// The difficulty settings, read from a .cfg file as one block.
typedef struct DifficultyCfg {
	uint8_t m_unlimitedAmmo;   // 0x00 — the "cia" cheat; cleared in network games with more than one player
	uint8_t m_invulnerable;    // 0x01 — the "blorb" cheat; cleared in network games
	uint8_t m_splashDamage;    // 0x02 — splash damage hurts mechs; set in network games with more than one player
	uint8_t m_collisionDamage; // 0x03 — collisions hurt mechs; set in network games with more than one player
	uint8_t m_heatTracking;    // 0x04 — fires heat mechs nearby; set in network games with more than one player
	uint8_t m_enemySkill;      // 0x05 — 0 easy, 1 medium, 2 hard (MW2SHELL); 2 in network games
	uint8_t m_reserved0x06[2]; // 0x06
	uint8_t m_regenerate;      // 0x08 — after the mission ends, the local player can regenerate
	uint8_t
		m_radar; // 0x09 — the radar and auto targeting (NETMECHW's "Radar + Auto Targeting"); set outside network games
	uint8_t m_teamGame;     // 0x0a
	uint32_t m_timeOfDay;   // 0x0b — the time of day phase a network game starts at; cleared outside network games
	uint32_t m_gravity;     // 0x0f — overrides the planet's gravity when set; cleared outside network games
	uint32_t m_temperature; // 0x13 — overrides the planet's temperature when set; cleared outside network games
} DifficultyCfg;
#pragma pack()

// The functions and globals of config.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_hudLayoutValues[3];
	extern int32_t g_torsoTwistDegrees;
	extern int32_t g_headingDegrees;
	extern CockpitPanel* g_cockpitPanels[c_panelCount];
	extern int32_t g_cockpitPanelEnabled[c_panelCount];
	extern int32_t g_cockpitPowerState;
	extern struct PANE g_cockpitPanelPanes[c_panelCount];
	extern int32_t g_punchInAutoHeadingRequested;
	extern char g_gameDir[256];
	extern int32_t g_hitFadePending;
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
	extern uint32_t g_cockpitPanelLightUpTimes[c_panelCount];
	extern int32_t g_lastWarningPowerState;
	extern int32_t g_lockedTonePlayed;
	extern int32_t g_lockingTonePlayed;
	extern int32_t g_hitFadeCount;
	extern char g_gamePath[0x50];

	int32_t LoadFile(char* p_path, int32_t* p_size, void** p_data, uint32_t* p_poolTag);
	int32_t LoadDifficultyCfg(char* p_name, DifficultyCfg** p_cfg);
	void LoadSndCfg(SoundConfig* p_cfg);
	void SaveSndCfg(const SoundConfig* p_cfg);
	char* BuildGamePath(char* p_name);
	void LayoutWeaponPanels(struct Mech* p_mech);
	void ScaleCockpitLayout(void);
	void InitCockpitPanels(void);
	void ResetCockpitPanels(void);
	void UpdateCockpit(struct Mech* p_mech);
	void ShutdownCockpitPanels(void);
	void DamageCockpitPanels(struct Mech* p_mech, int32_t p_heavy);
	void PlayCockpitWarnings(struct Mech* p_mech);
	void DrawPanelAnim(struct PANE* p_target, int32_t p_index, int32_t p_x, int32_t p_y);
	int32_t LoadMgdFile(
		struct ResourceRef* p_ref,
		int32_t* p_height,
		int32_t* p_cockpitHeight,
		int32_t* p_maxTorsoTwist,
		int32_t* p_radius
	);
	int32_t LoadReels(struct ResourceRef* p_ref);
	int32_t LoadHudFile(struct ResourceRef* p_ref);
	int32_t LoadCptFile(struct ResourceRef* p_ref, struct PANE* p_gauges, struct PANE* p_panels);
	void PreloadCockpitSounds(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H
