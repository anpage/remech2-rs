#include "video.h"

#include "audiosubsystem.h"
#include "decomp.h"
#include "displaybackend.h"
#include "files.h"
#include "fmvslot.h"
#include "keyboardinput.h"
#include "loopingmovie.h"
#include "mousestate.h"
#include "mss.h"
#include "readfile.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "vfxa.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// GLOBAL: MW2SHELL 0x100641a8
FmvSlot g_fmvSlots[32] = {0};

// GLOBAL: MW2SHELL 0x10064b2c
MechS32 g_fullscreenVideoMsg = c_msgScreenFrame;

// GLOBAL: MW2SHELL 0x10064b30
MechS32 g_fullscreenVideoWParam = c_msgScreenFrame;

// GLOBAL: MW2SHELL 0x1007cdd0
MechChar g_videoPath[0x20];

// GLOBAL: MW2SHELL 0x1007cdf0
MechChar g_shpPath[0x20];

// FUNCTION: MW2SHELL 0x10015d30
MechChar* GetPathToVideo(const MechChar* p_name)
{
	sprintf(g_videoPath, "smk\\%s.smk", p_name);
	return g_videoPath;
}

// FUNCTION: MW2SHELL 0x10015da1
MechChar* GetPathToShp(const MechChar* p_name)
{
	sprintf(g_shpPath, "smk\\%s.shp", p_name);
	return g_shpPath;
}

// The original looked for the video on the game CD when it wasn't on the hard disk, here and
// wherever a video or its SHP animation is opened.
// FUNCTION: MW2SHELL 0x10015e12
BOOL CheckVideoExists(const MechChar* p_name)
{
	return MechFileExists(GetPathToVideo(p_name));
}

// The screen callback while a full-screen video plays: a click, a key or any other message ends
// it.
// FUNCTION: MW2SHELL 0x10015e8e
void FullscreenVideoCallback(TMPackDataBase*, MechS32*, MechU8*, char**, MechS32 p_msg)
{
	FmvSlot* video;
	MechS32 msg;

	video = &g_fmvSlots[0];
	if (!IsVideoPlaying(0) || g_mouseState->GetLeftPressed() == 1 || g_keyboardInput->PollKey() ||
		p_msg != c_msgScreenFrame) {
		CloseVideo(0);
		UnregisterScreenFunction(FullscreenVideoCallback);
		if (p_msg == c_msgScreenFrame) {
			msg = g_fullscreenVideoMsg;
		}
		else {
			msg = p_msg;
		}

		PostMessage(g_gameWindow, msg, g_fullscreenVideoWParam, 0);
		g_fullscreenVideoMsg = g_fullscreenVideoWParam = c_msgScreenFrame;
		g_drawFmv = FALSE;
	}
}

// FUNCTION: MW2SHELL 0x10015f58
MechS32 BeginFullscreenVideo(const char* p_name, MechS32 p_msg, MechS32 p_wParam)
{
	MechS32 result;

	CloseAllVideos();
	result = PlayVideo(0, p_name, 0, 0, 0x1000, 0);
	if (result == -1) {
		PostMessage(g_gameWindow, p_msg, p_wParam, 0);
		return 0;
	}

	if (g_littleMovies != 0) {
		g_videoDriver->ActivateFramebuffer();
	}
	g_fullscreenVideoMsg = p_msg;
	g_fullscreenVideoWParam = p_wParam;
	RegisterScreenFunction(FullscreenVideoCallback);
	return 1;
}

// FUNCTION: MW2SHELL 0x10015fed
MechS32 PlayFullscreenVideo(const char* p_name, MechS32 p_msg, MechS32 p_wParam)
{
	if (BeginFullscreenVideo(p_name, p_msg, p_wParam) == 0) {
		return 0;
	}

	DisableShellMenu(g_windowMenu);
	g_drawFmv = TRUE;
	return 1;
}

DECOMP_SIZE_ASSERT(LoopingMovie, 0x18)

// Opens the movie p_name and draws its first
// frame at (p_left, p_top). Without a framebuffer it closes the movie again.
// FUNCTION: MW2SHELL 0x1001603a
LoopingMovie::LoopingMovie(MechChar* p_name, MechS32 p_left, MechS32 p_top)
{
	MechS32 result;

	m_smack = SmackOpen(GetPathToVideo(p_name), g_movieOpenFlags, 0);
	if (m_smack == NULL) {
		return;
	}

	m_left = p_left;
	m_top = p_top;
	m_width = m_smack->Width;
	m_height = m_smack->Height;
	m_frame = 1;

	if (g_windowActive != 0) {
		result = g_currentDisplayBackend->m_acquireFramebuffer();
	}
	else {
		result = -1;
	}

	if (result == 0) {
		SmackToBuffer(m_smack, m_left, m_top, 0x280, 0x1e0, g_videoDriver->m_screenBuffer.m_buffer, 0);
		SmackDoFrame(m_smack);
		g_videoDriver->ExpandRectBySize(m_left, m_top, m_width, m_height);
	}
	else {
		SmackClose(m_smack);
		m_smack = NULL;
	}
}

// FUNCTION: MW2SHELL 0x100161a8
LoopingMovie::~LoopingMovie()
{
	if (m_smack == NULL) {
		return;
	}

	SmackClose(m_smack);
}

// Moves the movie: restores the background under the old rectangle and draws the current
// frame at the new position.
// FUNCTION: MW2SHELL 0x100161dd
void LoopingMovie::MoveTo(MechS32 p_left, MechS32 p_top)
{
	MechS32 result;

	if (m_smack == NULL) {
		return;
	}

	g_videoDriver->RestoreBackground(m_left, m_top, m_width, m_height);
	m_left = p_left;
	m_top = p_top;
	if (g_windowActive != 0) {
		result = g_currentDisplayBackend->m_acquireFramebuffer();
	}
	else {
		result = -1;
	}

	if (result == 0) {
		SmackToBuffer(m_smack, m_left, m_top, 0x280, 0x1e0, g_videoDriver->m_screenBuffer.m_buffer, 0);
		SmackDoFrame(m_smack);
		g_videoDriver->ExpandRectBySize(m_left, m_top, m_width, m_height);
	}
}

// Allocates p_size bytes and drops the pointer; FUN_100162ef frees a block. Neither is called,
// and the bodies don't say what they served (a codec's allocation callbacks?), so they keep
// their placeholders.
// FUNCTION: MW2SHELL 0x100162d3
void FUN_100162d3(size_t p_size)
{
	malloc(p_size);
}

// Frees p_block. See FUN_100162d3.
// FUNCTION: MW2SHELL 0x100162ef
void FUN_100162ef(void* p_block)
{
	free(p_block);
}

// FUNCTION: MW2SHELL 0x1001630b
void LoopingMovie::Update()
{
	if (m_smack == NULL) {
		return;
	}
	if (SmackWait(m_smack)) {
		return;
	}

	if ((g_windowActive ? g_currentDisplayBackend->m_acquireFramebuffer() : -1) == 0) {
		m_frame++;
		if (m_smack->Frames < m_frame) {
			m_frame = 1;
			SmackGoto(m_smack, m_frame);
		}
		else {
			SmackNextFrame(m_smack);
		}
		SmackDoFrame(m_smack);
		g_videoDriver->ExpandRectBySize(m_left, m_top, m_width, m_height);
	}
}

// FUNCTION: MW2SHELL 0x100163ff
MechS32 IsVideoFrameDue(FmvSlot* p_video, MechU32 p_time)
{
	if (p_time >= p_video->m_nextFrameTime) {
		p_video->m_nextFrameTime = p_video->m_frameInterval + p_time;
		return 0;
	}
	else {
		return -1;
	}
}

// The original also streamed the video's sound track to a VideoSound object here, but no slot
// ever had one: Smacker plays the sound itself.
// FUNCTION: MW2SHELL 0x1001643e
void StreamVideoSound(FmvSlot* p_video)
{
	AIL_serve();
}

// Plays the next frame of the full-screen video in slot 0, closing it after the last.
// Stack-slot permutation: video, palette and result.
// FUNCTION: MW2SHELL 0x100164f2
void PlayFullscreenVideoFrame()
{
	MechS32 result;
	PaletteColor* palette;
	FmvSlot* video;

	video = &g_fmvSlots[0];
	if (!SmackWait(video->m_smack)) {
		if (video->m_frame == 1) {
			g_videoDriver->m_fmvFirstFrame = 1;
		}

		if (video->m_smack->NewPalette) {
			if (video->m_smack->PalType == 1) {
				palette = (PaletteColor*) video->m_smack->Palette;
			}
			else {
				palette = (PaletteColor*) video->m_smack->AlternatePalette;
			}
			g_videoDriver->SetPalette(palette, 0);
		}

		if (g_windowActive) {
			result = g_currentDisplayBackend->m_acquireFramebuffer();
		}
		else {
			result = -1;
		}

		if (result == 0) {
			SmackDoFrame(video->m_smack);
			g_videoDriver->ExpandRectBySize(0, 0, video->m_width, video->m_height);
		}

		video->m_frame++;
		if (video->m_frame > video->m_frameCount) {
			CloseVideo(0);
		}
		else {
			SmackNextFrame(video->m_smack);
		}
	}
}

// Advances and draws the videos of all slots: Smacker videos frame by frame, sprite sheets
// (m_shp) on their timer. A video past its last frame stops (flag 4), loops (flag 8) or
// closes.
// Not 100%: the stack slots of video and i are permuted, and m_drawnTop != m_top loads m_drawnTop
// first (reversing the operands or retyping the member doesn't flip it).
// FUNCTION: MW2SHELL 0x1001661b
void UpdateVideos()
{
	FmvSlot* video;
	MechS32 i;

	if (g_audioSubsystem) {
		AIL_serve();
	}

	if (g_menuDialogOpen) {
		return;
	}

	video = g_fmvSlots;
	if (video->m_flags & 0x80000000 && video->m_flags & 0x1000) {
		PlayFullscreenVideoFrame();
		return;
	}

	for (i = 0, video = g_fmvSlots; i < 32; i++, video++) {
		if (video->m_flags & 0x80000000) {
			if (video->m_smack && !video->m_frame && !(video->m_flags & 1)) {
				SmackDoFrame(video->m_smack);
				if (video->m_flags & 2) {
					video->m_flags |= 0x10;
				}
				video->m_frame = 1;
			}

			if (video->m_smack && !(video->m_flags & 2)) {
				video->m_drawnFrame = video->m_frame;
			}

			if (video->m_drawnLeft != video->m_left || video->m_drawnTop != video->m_top ||
				video->m_drawnFrame != video->m_frame || video->m_flags & 0x40000020) {
				if (video->m_flags & 0x10) {
					g_videoDriver
						->RestoreBackground(video->m_drawnLeft, video->m_drawnTop, video->m_width, video->m_height);
				}
				video->m_flags |= 0x100;
			}

			video->m_drawnLeft = video->m_left;
			video->m_drawnTop = video->m_top;
			video->m_drawnFrame = video->m_frame;
			video->m_flags &= ~0x10;
			if (video->m_flags & 0x40000000) {
				CloseVideo(i);
			}
		}
	}

	g_videoDriver->RedrawGlyphs(0);
	for (i = 0, video = g_fmvSlots; i < 32; i++, video++) {
		if (video->m_flags & 0x80000000) {
			if (video->m_smack) {
				if (video->m_frameBuffer && !(video->m_flags & 0x20)) {
					video->m_flags |= 0x10;
					if (video->m_flags & 0x100) {
						g_videoDriver->DrawPixels(
							(undefined*) video->m_frameBuffer,
							video->m_left,
							video->m_top,
							video->m_width,
							video->m_height
						);
					}
					else {
						g_videoDriver->DrawPixelsClipped(
							(undefined*) video->m_frameBuffer,
							video->m_left,
							video->m_top,
							video->m_width,
							video->m_height
						);
					}
					video->m_flags &= ~0x100;
				}

				if (!(video->m_flags & 1) && !SmackWait(video->m_smack)) {
					video->m_frame++;
					if (video->m_frame > video->m_frameCount) {
						video->m_frame--;
						if (video->m_flags & 4) {
							video->m_flags |= 1;
							continue;
						}
						else if (video->m_flags & 8) {
							video->m_frame = 1;
							SmackGoto(video->m_smack, video->m_frame);
						}
						else {
							video->m_flags |= 0x40000001;
							continue;
						}
					}
					else if (video->m_frame != 1) {
						SmackNextFrame(video->m_smack);
					}

					SmackDoFrame(video->m_smack);
					video->m_flags |= 0x100;
					if (video->m_flags & 2) {
						video->m_flags |= 0x10;
					}
				}
			}
			else if (video->m_shp) {
				if (!(video->m_flags & 0x20)) {
					video->m_flags |= 0x10;
					if (video->m_flags & 0x100) {
						g_videoDriver->DrawShpFrame(
							video->m_shp,
							video->m_frame,
							video->m_left,
							video->m_top,
							video->m_width,
							video->m_height
						);
					}
					else {
						g_videoDriver->DrawShpFrameClipped(
							video->m_shp,
							video->m_frame,
							video->m_left,
							video->m_top,
							video->m_width,
							video->m_height
						);
					}
					video->m_flags &= ~0x100;
				}

				if (!(video->m_flags & 1) && !IsVideoFrameDue(video, timeGetTime())) {
					video->m_frame++;
					if (video->m_frame >= video->m_frameCount) {
						video->m_frame--;
						if (video->m_flags & 4) {
							video->m_flags |= 1;
						}
						else if (video->m_flags & 8) {
							video->m_frame = 0;
						}
						else {
							video->m_flags |= 0x40000001;
						}
					}
					video->m_flags |= 0x100;
				}
			}
		}
	}
	g_videoDriver->RedrawGlyphs(1);
}

// FUNCTION: MW2SHELL 0x10016b11
MechS32 IsVideoPlaying(MechS32 p_index)
{
	if (p_index >= 0 && p_index < 0x20 && (g_fmvSlots[p_index].m_flags & 0x80000000) &&
		!(g_fmvSlots[p_index].m_flags & 1)) {
		return 1;
	}
	else {
		return 0;
	}
}

// Whether any video plays Smacker's sound (flag 0x2000). Unused.
// FUNCTION: MW2SHELL 0x10016b78
MechS32 IsVideoSoundPlaying()
{
	MechS32 i;

	for (i = 0; i < 0x20; i++) {
		if ((g_fmvSlots[i].m_flags & 0x80000000) && (g_fmvSlots[i].m_flags & 0x2000)) {
			return 1;
		}
	}

	return 0;
}

// FUNCTION: MW2SHELL 0x10016be7
MechS32 IsFullscreenVideoPlaying()
{
	if ((g_fmvSlots[0].m_flags & 0x80000000) && (g_fmvSlots[0].m_flags & 0x1000)) {
		return 1;
	}

	return 0;
}

// Pauses the full-screen video when the shell loses the focus: steps back a frame and keeps the
// screen as the background.
// FUNCTION: MW2SHELL 0x10016c1d
void PauseFullscreenVideo()
{
	g_fmvSlots[0].m_frame--;
	g_videoDriver->CopyScreenToBackground();
}

// Resumes the full-screen video when the shell gets the focus back: restores the screen and
// redraws the frame PauseFullscreenVideo stepped back to.
// FUNCTION: MW2SHELL 0x10016c3e
void ResumeFullscreenVideo()
{
	MechS32 result;

	SmackGoto(g_fmvSlots[0].m_smack, g_fmvSlots[0].m_frame);
	g_videoDriver->CopyBackgroundToScreen();
	if (g_windowActive != 0) {
		result = g_currentDisplayBackend->m_acquireFramebuffer();
	}
	else {
		result = -1;
	}

	if (result == 0) {
		SmackDoFrame(g_fmvSlots[0].m_smack);
	}

	SmackNextFrame(g_fmvSlots[0].m_smack);
	g_fmvSlots[0].m_frame++;
}

// The original loads p_mask before p_value in p_mask & p_value; swapping the operands didn't flip it.
// The first statement reloads and stores the flags instead of and-ing them in place: an unsigned
// operation on a signed field does that, which is why the mask is cast.
// FUNCTION: MW2SHELL 0x10016cc0
void SetVideoFlags(MechS32 p_index, MechS32 p_mask, MechS32 p_value)
{
	if (p_index >= 0 && p_index < 0x20) {
		g_fmvSlots[p_index].m_flags &= ~(MechU32) p_mask;
		g_fmvSlots[p_index].m_flags |= p_mask & p_value;
	}
}

// Shows a video opened hidden (flag 0x20), redrawing it.
// FUNCTION: MW2SHELL 0x10016d27
void ShowVideo(MechS32 p_index)
{
	if (p_index >= 0 && p_index < 0x20 && (g_fmvSlots[p_index].m_flags & 0x20)) {
		g_fmvSlots[p_index].m_flags = g_fmvSlots[p_index].m_flags & ~0x20 | 0x100;
	}
}

// FUNCTION: MW2SHELL 0x10016d90
void CloseVideo(MechS32 p_index)
{
	if (p_index < 0 || p_index >= 0x20) {
		return;
	}

	if (g_fmvSlots[p_index].m_flags & 0x1000) {
		EnableShellMenu(g_windowMenu);
	}

	if (g_fmvSlots[p_index].m_smack != NULL) {
		SmackClose(g_fmvSlots[p_index].m_smack);
	}

	if (g_fmvSlots[p_index].m_shp != NULL) {
		MechHeapFree(g_primaryHeap, g_fmvSlots[p_index].m_shp);
	}

	if (g_fmvSlots[p_index].m_frameBuffer != NULL) {
		MechHeapFree(g_primaryHeap, g_fmvSlots[p_index].m_frameBuffer);
	}

	g_fmvSlots[p_index].m_smack = NULL;
	g_fmvSlots[p_index].m_shp = NULL;
	ZeroMemory(&g_fmvSlots[p_index].m_flags, 4);
	g_fmvSlots[p_index].m_frameBuffer = NULL;
}

// FUNCTION: MW2SHELL 0x10016f45
void CloseAllVideos()
{
	MechS32 i;

	for (i = 0; i < 0x20; i++) {
		CloseVideo(i);
	}
}

// FUNCTION: MW2SHELL 0x10016f82
void MoveVideo(MechS32 p_index, MechS32 p_left, MechS32 p_top)
{
	if (p_index >= 0 && p_index < 0x20 && (g_fmvSlots[p_index].m_flags & 0x80000000)) {
		if (g_fmvSlots[p_index].m_flags & 0x80) {
			p_left -= g_fmvSlots[p_index].m_width / 2;
			p_top -= g_fmvSlots[p_index].m_height;
		}

		g_fmvSlots[p_index].m_left = p_left;
		g_fmvSlots[p_index].m_top = p_top;
	}
}

// Opens the video p_name in the slot. A video with sound that the audio subsystem can play gets
// Smacker's sound (flag 0x2000). With flag 0x1000 the first frame goes straight to the screen, with
// flag 2 to the back buffer, otherwise to a buffer of its own.
// FUNCTION: MW2SHELL 0x1001703b
BOOL LoadVideoFile(FmvSlot* p_slot, const MechChar* p_name)
{
	MechS32 result;

	p_slot->m_smack = SmackOpen(GetPathToVideo(p_name), ((p_slot->m_flags & 0x40) >> 1) | 0xfe00, 0);
	if (!p_slot->m_smack) {
		return FALSE;
	}

	if (g_audioSubsystem && SmackSoundInTrack(p_slot->m_smack, 0x200)) {
		SmackClose(p_slot->m_smack);
		g_audioSubsystem->CloseDigitalDriver();
		p_slot->m_smack = SmackOpen(GetPathToVideo(p_name), ((p_slot->m_flags & 0x40) >> 1) | 0xfe00, 0);
		if (!p_slot->m_smack) {
			return FALSE;
		}

		p_slot->m_flags |= 0x2000;
	}
	else {
		SmackClose(p_slot->m_smack);
		p_slot->m_smack = SmackOpen(GetPathToVideo(p_name), (p_slot->m_flags & 0x40) >> 1, 0);
		if (!p_slot->m_smack) {
			return FALSE;
		}
	}

	p_slot->m_width = p_slot->m_smack->Width;
	p_slot->m_height = p_slot->m_smack->Height;
	if (p_slot->m_flags & 0x80) {
		p_slot->m_left -= p_slot->m_width / 2;
		p_slot->m_top -= p_slot->m_height;
	}

	p_slot->m_frame = 0;
	p_slot->m_frameCount = p_slot->m_smack->Frames;
	p_slot->m_drawnFrame = p_slot->m_frame;
	p_slot->m_frameBuffer = NULL;

	if (p_slot->m_flags & 0x1000) {
		p_slot->m_frame = 1;
		if (g_windowActive != 0) {
			result = g_currentDisplayBackend->m_acquireFramebuffer();
		}
		else {
			result = -1;
		}

		if (result == 0) {
			SmackToBuffer(
				p_slot->m_smack,
				p_slot->m_left,
				p_slot->m_top,
				0x280,
				0x1e0,
				g_videoDriver->m_screenBuffer.m_buffer,
				0
			);
		}
		else {
			SmackClose(p_slot->m_smack);
			return FALSE;
		}
	}
	else if (p_slot->m_flags & 2) {
		SmackToBuffer(
			p_slot->m_smack,
			p_slot->m_left,
			p_slot->m_top,
			0x280,
			0x1e0,
			g_videoDriver->m_backBuffer.m_buffer,
			0
		);
	}
	else {
		p_slot->m_frameBuffer = MechHeapAlloc(g_primaryHeap, p_slot->m_width * p_slot->m_height);
		SmackToBuffer(p_slot->m_smack, 0, 0, p_slot->m_width, p_slot->m_height, p_slot->m_frameBuffer, 0);
	}

	return TRUE;
}

// Loads the SHP animation p_name into the slot.
// FUNCTION: MW2SHELL 0x10017376
BOOL LoadShpFile(FmvSlot* p_slot, const MechChar* p_name)
{
	MechS32 size;

	p_slot->m_shp = MechReadFile(g_primaryHeap, GetPathToShp(p_name));
	if (!p_slot->m_shp) {
		return FALSE;
	}

	size = VFX_shape_bounds(p_slot->m_shp, 0);
	p_slot->m_width = (size >> 16) + 1;
	p_slot->m_height = (size & 0xffff) + 1;
	if (p_slot->m_flags & 0x80) {
		p_slot->m_left -= p_slot->m_width / 2;
		p_slot->m_top -= p_slot->m_height;
	}

	p_slot->m_frame = 0;
	p_slot->m_frameCount = VFX_shape_count(p_slot->m_shp);
	p_slot->m_drawnFrame = p_slot->m_frame;
	p_slot->m_frameBuffer = NULL;
	p_slot->m_nextFrameTime = 0;

	return TRUE;
}

// Plays the video or SHP animation p_name in slot p_index at (p_left, p_top), p_fps frames a
// second (10 for 0), replacing what the slot played. Returns the slot, or -1.
// FUNCTION: MW2SHELL 0x10017460
MechS32 PlayVideo(
	MechS32 p_index,
	const char* p_name,
	undefined4 p_left,
	undefined4 p_top,
	MechU32 p_flags,
	MechU32 p_fps
)
{
	FmvSlot* slot = &g_fmvSlots[p_index];

	if (p_index < 0 || p_index >= 0x20) {
		return -1;
	}

	if (slot->m_flags & 0x80000000) {
		if (slot->m_flags & 0x10) {
			g_videoDriver->RestoreBackground(slot->m_drawnLeft, slot->m_drawnTop, slot->m_width, slot->m_height);
		}
		CloseVideo(p_index);
	}

	slot->m_left = p_left;
	slot->m_top = p_top;
	slot->m_flags = p_flags | 0x80000100;
	if (!p_fps) {
		p_fps = 10;
	}
	slot->m_frameInterval = 1000 / p_fps;

	if (CheckVideoExists(p_name)) {
		if (!LoadVideoFile(slot, p_name)) {
			slot->m_flags = 0;
			return -1;
		}
	}
	else if (!LoadShpFile(slot, p_name)) {
		slot->m_flags = 0;
		return -1;
	}

	slot->m_drawnLeft = slot->m_left;
	slot->m_drawnTop = slot->m_top;

	return p_index;
}

// Plays a video in the first free slot.
// FUNCTION: MW2SHELL 0x100175e2
MechS32 PlayVideoInFreeSlot(MechChar* p_name, MechS32 p_left, MechS32 p_top, MechU32 p_flags, MechU32 p_fps)
{
	MechS32 i;

	for (i = 0; i < 0x20; i++) {
		if (!(g_fmvSlots[i].m_flags & 0x80000000)) {
			return PlayVideo(i, p_name, p_left, p_top, p_flags, p_fps);
		}
	}

	return -1;
}

// FUNCTION: MW2SHELL 0x10017656
MechS32 GetVideoFrame(MechS32 p_index)
{
	if (p_index >= 0 && p_index < 0x20) {
		return g_fmvSlots[p_index].m_frame;
	}
	else {
		return 0;
	}
}

// FUNCTION: MW2SHELL 0x10017698
void SetVideoFrame(MechS32 p_index, MechS32 p_frame)
{
	if (p_index >= 0 && p_index < 0x20) {
		if (p_frame >= g_fmvSlots[p_index].m_frameCount) {
			p_frame = 0;
		}

		g_fmvSlots[p_index].m_frame = p_frame;
		g_fmvSlots[p_index].m_drawnFrame = -1;
	}
}
