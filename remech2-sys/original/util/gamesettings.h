#ifndef GAMESETTINGS_H
#define GAMESETTINGS_H

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// Volumes as fixed point numbers, 0 to 0x10000
	typedef struct SoundSettings {
		MechS32 m_effectsVolume;
		MechS32 m_voiceVolume;
		MechS32 m_musicVolume;
	} SoundSettings;

	// Each 0 or 1, except m_brightness (0-15)
	typedef struct DisplaySettings {
		MechS32 m_objectTextmaps;
		MechS32 m_terrainTextmaps;
		MechS32 m_highDetail;
		MechS32 m_highObjectDensity;
		MechS32 m_explosionChunks;
		MechS32 m_brightness;
	} DisplaySettings;

	// Each 0 or 1, except m_enemySkill: 0 easy, 1 medium, 2 hard
	typedef struct DifficultySettings {
		MechU8 m_unlimitedAmmo;
		MechU8 m_invulnerable;
		MechU8 m_splashDamage;
		MechU8 m_collisionDamage;
		MechU8 m_heatTracking;
		MechU8 m_enemySkill;
	} DifficultySettings;

	// The settings in remech2.toml, implemented on the Rust side (src/settings.rs)
	void MechGetSoundSettings(SoundSettings* p_settings);
	void MechSetSoundSettings(const SoundSettings* p_settings);
	void MechGetDisplaySettings(DisplaySettings* p_settings);
	void MechSetDisplaySettings(const DisplaySettings* p_settings);
	void MechGetDifficultySettings(DifficultySettings* p_settings);
	void MechSetDifficultySettings(const DifficultySettings* p_settings);

#ifdef __cplusplus
}
#endif

#endif // GAMESETTINGS_H
