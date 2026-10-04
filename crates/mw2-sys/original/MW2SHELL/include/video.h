#ifndef VIDEO_H
#define VIDEO_H

#include "decomp.h"
#include "types.h"

// The functions and globals of video.cpp that other units use.
MechS32 BeginFullscreenVideo(const char* p_name, MechS32 p_msg, MechS32 p_wParam);
MechS32 PlayFullscreenVideo(const char* p_name, MechS32 p_msg, MechS32 p_wParam);
void UpdateVideos();
MechS32 IsVideoPlaying(MechS32 p_index);
MechS32 IsFullscreenVideoPlaying();
void PauseFullscreenVideo();
void ResumeFullscreenVideo();
void SetVideoFlags(MechS32 p_index, MechS32 p_mask, MechS32 p_value);
void ShowVideo(MechS32 p_index);
void CloseVideo(MechS32 p_index);
void CloseAllVideos();
void MoveVideo(MechS32 p_index, MechS32 p_left, MechS32 p_top);
MechS32 PlayVideoInFreeSlot(MechChar* p_name, MechS32 p_left, MechS32 p_top, MechU32 p_flags, MechU32 p_fps);
void SetVideoFrame(MechS32 p_index, MechS32 p_frame);
MechS32 PlayVideo(
	MechS32 p_index,
	const char* p_name,
	undefined4 p_left,
	undefined4 p_top,
	MechU32 p_flags,
	MechU32 p_fps
);

#endif // VIDEO_H
