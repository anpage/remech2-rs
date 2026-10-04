#ifndef SUPANIM_H
#define SUPANIM_H

#include "types.h"

// The functions and globals of supanim.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechChar* g_supAnimBackdropName;
	extern MechChar* g_supAnimShapeName;

	void StartSupAnim(MechS32 p_slowFade);
	void StopSupAnim(void);

#ifdef __cplusplus
}
#endif

#endif // SUPANIM_H
