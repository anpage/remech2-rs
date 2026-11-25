#ifndef MECHDAMAGE_H
#define MECHDAMAGE_H

#include "decomp.h"
#include "types.h"

struct Mech;

// The functions and globals of mechdamage.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_autoEject;
	extern int32_t g_otherArmorPerLevel;
	extern int32_t g_localArmorPerLevel;
	extern int32_t g_killCount;
	extern int32_t g_localMechHidden;
	extern int32_t g_criticalHeatWarned;
	extern int32_t g_criticalHeatWarningTime;

	void RunAutopilot(struct Mech* p_mech);
	void PunchInAutoHeading(struct Mech* p_mech);
	void CalculateHeat(struct Mech* p_mech);
	void DestroyMech(int32_t p_killer, struct Mech* p_mech);
	void KillMech(int32_t p_killer, struct Mech* p_mech);
	void DestroySectionSlots(int32_t p_attacker, struct Mech* p_mech, uint32_t p_section);
	void DestroySection(int32_t p_attacker, struct Mech* p_mech, uint32_t p_section);
	void DestroyCriticalSlot(
		int32_t p_attacker,
		struct Mech* p_mech,
		uint32_t p_section,
		int32_t p_slot,
		int32_t p_recursing
	);
	void ApplyDamageToMech(int32_t p_attacker, struct Mech* p_mech, int32_t p_damage, int32_t p_section);
	void EjectPlayer(struct Mech* p_mech, int32_t p_eject);
	void ToggleLocalMechVisible(void);

#ifdef __cplusplus
}
#endif

#endif // MECHDAMAGE_H
