#ifndef MEKFILE_H
#define MEKFILE_H

struct MechSection;
struct MekHeader;
struct MekWeapon;
struct Mech;

#include "types.h"

// The functions and globals of mekfile.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern uint16_t g_weaponValues[30];

	int32_t LoadMechConfig(struct Mech* p_mech, char* p_name, int32_t p_id, char* p_config);
	uint16_t GetMechValue(struct MekHeader* p_header, struct MechSection* p_sections, struct MekWeapon* p_weapons);

#ifdef __cplusplus
}
#endif

#endif // MEKFILE_H
