#ifndef DIFFICULTYCONFIG_H
#define DIFFICULTYCONFIG_H

#include "decomp.h"
#include "types.h"

// SIZE 0x17
// The difficulty settings, read from and written to MW2DIF.CFG as one block. The options screen's
// rows point at the members; the simulator reads the same file.
struct DifficultyConfig {
	MechU8 m_unlimitedAmmo;           // 0x00 — dishonorable when set
	MechU8 m_invulnerability;         // 0x01 — dishonorable when set
	MechU8 m_splashDamage;            // 0x02 — the simulator's TOGGLE_SPLASH_DAMAGE key flips it
	MechU8 m_collisionDamage;         // 0x03 — dishonorable when clear
	MechU8 m_heatTracking;            // 0x04 — no honor when clear
	MechU8 m_enemySkill;              // 0x05 — 0 easy, 1 medium, 2 hard
	undefined m_unk0x06[0x17 - 0x06]; // 0x06 — the rest of the simulator's settings; the shell never accesses them
};

#endif // DIFFICULTYCONFIG_H
