#ifndef MEKFILE_H
#define MEKFILE_H

struct MechSection;
struct MekHeader;
struct MekWeapon;
struct Mech;

#include "decomp.h"
#include "types.h"

// The functions and globals of mekfile.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechU16 g_weaponValues[30];

	MechS32 LoadMechConfig(struct Mech* p_mech, MechChar* p_name, MechS32 p_id, MechChar* p_config);
	MechU16 GetMechValue(struct MekHeader* p_header, struct MechSection* p_sections, struct MekWeapon* p_weapons);

#ifdef __cplusplus
}
#endif

#endif // MEKFILE_H
