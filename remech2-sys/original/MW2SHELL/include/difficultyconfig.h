#ifndef DIFFICULTYCONFIG_H
#define DIFFICULTYCONFIG_H

#include "types.h"

// The difficulty settings.
// The options screen's rows point at the members; the simulator reads the same file.
struct DifficultyConfig {
	// dishonorable when set
	uint8_t m_unlimitedAmmo;
	// dishonorable when set
	uint8_t m_invulnerability;
	uint8_t m_splashDamage;
	// dishonorable when clear
	uint8_t m_collisionDamage;
	// no honor when clear
	uint8_t m_heatTracking;
	// 0 easy, 1 medium, 2 hard
	uint8_t m_enemySkill;
};

#endif // DIFFICULTYCONFIG_H
