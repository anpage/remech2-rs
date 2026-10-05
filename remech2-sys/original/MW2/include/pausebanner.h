#ifndef PAUSEBANNER_H
#define PAUSEBANNER_H

#include "pane.h"
#include "types.h"

// The functions and globals of pausebanner.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_debugSlot;
	extern MechS32 g_debugSection;
	extern PANE g_pausedBannerRect;
	extern MechS32 g_pausedBannerUnscaled;

	void DrawPausedBanner(void);
	void PlayPauseSound(void);
	void PlayResumeSound(void);
	void PauseGame(void);
	void ResumeGame(void);
	void HandleDebugKey(MechU16 p_key);

#ifdef __cplusplus
}
#endif

#endif // PAUSEBANNER_H
