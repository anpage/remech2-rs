#ifndef GAMESETTINGS_H
#define GAMESETTINGS_H

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// Volumes as fixed point numbers, 0 to 0x10000
	typedef struct SoundSettings {
		int32_t m_effectsVolume;
		int32_t m_voiceVolume;
		int32_t m_musicVolume;
	} SoundSettings;

	// Each 0 or 1, except m_brightness (0-15)
	typedef struct DisplaySettings {
		int32_t m_objectTextmaps;
		int32_t m_terrainTextmaps;
		int32_t m_highDetail;
		int32_t m_highObjectDensity;
		int32_t m_explosionChunks;
		int32_t m_brightness;
	} DisplaySettings;

	// Each 0 or 1, except m_enemySkill: 0 easy, 1 medium, 2 hard
	typedef struct DifficultySettings {
		uint8_t m_unlimitedAmmo;
		uint8_t m_invulnerable;
		uint8_t m_splashDamage;
		uint8_t m_collisionDamage;
		uint8_t m_heatTracking;
		uint8_t m_enemySkill;
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
