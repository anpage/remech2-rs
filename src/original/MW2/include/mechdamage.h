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

	extern MechS32 g_autoEject;
	extern MechS32 g_otherArmorPerLevel;
	extern MechS32 g_localArmorPerLevel;
	extern MechS32 g_killCount;

	void RunAutopilot(struct Mech* p_mech);
	void PunchInAutoHeading(struct Mech* p_mech);
	void CalculateHeat(struct Mech* p_mech);
	void DestroyMech(MechS32 p_killer, struct Mech* p_mech);
	void KillMech(MechS32 p_killer, struct Mech* p_mech);
	void DestroySectionSlots(MechS32 p_attacker, struct Mech* p_mech, MechU32 p_section);
	void DestroySection(MechS32 p_attacker, struct Mech* p_mech, MechU32 p_section);
	void DestroyCriticalSlot(
		MechS32 p_attacker,
		struct Mech* p_mech,
		MechU32 p_section,
		MechS32 p_slot,
		MechS32 p_recursing
	);
	void ApplyDamageToMech(MechS32 p_attacker, struct Mech* p_mech, MechS32 p_damage, MechS32 p_section);
	void EjectPlayer(struct Mech* p_mech, MechS32 p_eject);
	void ToggleLocalMechVisible(void);

#ifdef __cplusplus
}
#endif

#endif // MECHDAMAGE_H
