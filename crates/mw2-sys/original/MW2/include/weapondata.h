#ifndef WEAPONDATA_H
#define WEAPONDATA_H

#include "effectinfo.h"
#include "weapondef.h"

// The globals of weapondata.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern WeaponDef g_weaponDefs[31];
	extern EffectInfo g_effectInfo[0x20];

#ifdef __cplusplus
}
#endif

#endif // WEAPONDATA_H
