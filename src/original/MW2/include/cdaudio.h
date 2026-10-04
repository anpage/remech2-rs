#ifndef CDAUDIO_H
#define CDAUDIO_H

#include "decomp.h"
#include "types.h"

// The state of the CD drive, as GetCdStatus reports it.
enum {
	c_cdStatusOpen = 1,
	c_cdStatusStopped = 2,
	c_cdStatusPlaying = 3,
	c_cdStatusPaused = 4,
	c_cdStatusError = 5
};

// SIZE 0xc
// The disc's table of contents: the start of every track, as a TMSF position, and an end marker.
typedef struct CdAudioTracks {
	MechS32 m_firstTrack;      // 0x00
	MechS32 m_numberOfTracks;  // 0x04
	MechU32* m_trackPositions; // 0x08
} CdAudioTracks;

// SIZE 0x10
typedef struct CdAudioPosition {
	MechS32 m_track;  // 0x00
	MechS32 m_minute; // 0x04
	MechS32 m_second; // 0x08
	MechS32 m_frame;  // 0x0c
} CdAudioPosition;

// The functions and globals of cdaudio.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 GetCdAudioAuxDevice(void);
	MechS32 InitCdAudio(void);
	MechS32 CloseCdAudio(void);
	void PauseCdAudio(void);
	void ResumeCdAudio(void);
	void StopCdAudio(void);
	void PlayCdAudio(MechU32 p_from, MechU32 p_to);
	MechS32 StartCdAudio(void);
	void DeInitCdAudio(void);
	MechS32 GetCdStatus(void);
	MechS32 RefreshCdStatus(void);
	void OpenCdDoor(void);
	void CdAudioTogglePaused(void);
	void ContinueCdAudio(void);
	void StopCdAudioAndWait(void);
	void PreviousCdTrack(void);
	void NextCdTrack(void);
	void UpdateCdAudioPosition(CdAudioPosition* p_position);
	undefined4 FUN_1005afbb(void);
	void FUN_1005afd0(undefined4 p_unk0x00);
	undefined4 FUN_1005afe3(void);
	void FUN_1005aff8(undefined4 p_unk0x00);
	MechS32 PollCdDrive(void);
	void SkipCdTracks(CdAudioTracks* p_tracks, MechS32 p_step);
	void PlayCdTrack(MechS32 p_track);
	void PlayNewCdAudio(MechU32 p_from, MechU32 p_to);
	MechS32 GetCdAudioTracks(CdAudioTracks* p_tracks);
	void FreeCdAudioTracks(CdAudioTracks* p_tracks);
	void GetCdAudioPosition(CdAudioPosition* p_position);
	MechS32 IsCdAudioInitialized(void);
	MechS32 IsCdTrackOnDisc(MechS32 p_track);
	MechS32 GetCdAudioVolume(void);
	MechS32 SetCdAudioVolume(MechS32 p_volume);
	void ApplyCdAudioVolume(void);

#ifdef __cplusplus
}
#endif

#endif // CDAUDIO_H
