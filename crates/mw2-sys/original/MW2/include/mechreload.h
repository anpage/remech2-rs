#ifndef MECHRELOAD_H
#define MECHRELOAD_H

#include "decomp.h"
#include "mech.h"
#include "mechsegment.h"
#include "object.h"
#include "rememberedmech.h"
#include "types.h"

// The functions and globals of mechreload.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_reloadingPlayer;
	extern MechS32 g_unk0x100ba694;
	extern RememberedMech g_rememberedMechs[60];
	extern MechSegment* g_mechSegments[60];

	// Declared without a prototype: network.c calls it with two more (zero) arguments.
	MechS32 ReloadPlayerMech();
	void RememberLoadMech(Mech* p_mech, MechChar* p_name, MechS32 p_id, MechChar* p_config);
	void RememberMechSegments(Mech* p_mech);
	SceneObject* RestoreMechSegments(MechSegment* p_segment);
	MechSegment* SaveMechSegments(SceneObject* p_obj);

#ifdef __cplusplus
}
#endif

#endif // MECHRELOAD_H
