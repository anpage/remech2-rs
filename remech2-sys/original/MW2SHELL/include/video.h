#ifndef VIDEO_H
#define VIDEO_H

#include "decomp.h"
#include "fmvslot.h"
#include "types.h"

// The functions and globals of video.cpp that other units use.
extern FmvSlot g_fmvSlots[32];
extern int32_t g_fullscreenVideoMsg;
extern int32_t g_fullscreenVideoWParam;
extern char g_videoPath[0x20];
extern char g_shpPath[0x20];
int32_t BeginFullscreenVideo(const char* p_name, int32_t p_msg, int32_t p_wParam);
int32_t PlayFullscreenVideo(const char* p_name, int32_t p_msg, int32_t p_wParam);
void UpdateVideos();
int32_t IsVideoPlaying(int32_t p_index);
int32_t IsFullscreenVideoPlaying();
void SetVideoFlags(int32_t p_index, int32_t p_mask, int32_t p_value);
void ShowVideo(int32_t p_index);
void CloseVideo(int32_t p_index);
void CloseAllVideos();
void MoveVideo(int32_t p_index, int32_t p_left, int32_t p_top);
int32_t PlayVideoInFreeSlot(char* p_name, int32_t p_left, int32_t p_top, uint32_t p_flags, uint32_t p_fps);
void SetVideoFrame(int32_t p_index, int32_t p_frame);
int32_t PlayVideo(
	int32_t p_index,
	const char* p_name,
	undefined4 p_left,
	undefined4 p_top,
	uint32_t p_flags,
	uint32_t p_fps
);

#endif // VIDEO_H
