#ifndef PAUSEBANNER_H
#define PAUSEBANNER_H

#include "pane.h"
#include "types.h"

// The functions and globals of pausebanner.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_debugSlot;
	extern int32_t g_debugSection;
	extern PANE g_pausedBannerRect;
	extern int32_t g_pausedBannerUnscaled;

	void DrawPausedBanner(void);
	void PlayPauseSound(void);
	void PlayResumeSound(void);
	void PauseGame(void);
	void ResumeGame(void);
	void HandleDebugKey(uint16_t p_key);

#ifdef __cplusplus
}
#endif

#endif // PAUSEBANNER_H
