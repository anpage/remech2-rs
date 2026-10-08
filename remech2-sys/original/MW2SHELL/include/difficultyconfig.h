#ifndef DIFFICULTYCONFIG_H
#define DIFFICULTYCONFIG_H

#include "types.h"

// The difficulty settings.
// The options screen's rows point at the members; the simulator reads the same file.
struct DifficultyConfig {
	// dishonorable when set
	MechU8 m_unlimitedAmmo;
	// dishonorable when set
	MechU8 m_invulnerability;
	// the simulator's TOGGLE_SPLASH_DAMAGE key flips it
	MechU8 m_splashDamage;
	// dishonorable when clear
	MechU8 m_collisionDamage;
	// no honor when clear
	MechU8 m_heatTracking;
	// 0 easy, 1 medium, 2 hard
	MechU8 m_enemySkill;
};

#endif // DIFFICULTYCONFIG_H
