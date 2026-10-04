#include "cdaudio.h"

#include "audio.h"
#include "gamecd.h"
#include "simmain.h"
#include "types.h"

#include <conio.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <windows.h>

// GLOBAL: MW2 0x100aa278
MCIDEVICEID g_cdAudioDevice = (MCIDEVICEID) -1;

// GLOBAL: MW2 0x100aa27c
MechS32 g_cdAudioAuxDevice = -1;

// GLOBAL: MW2 0x100aa280
CdAudioTracks g_cdAudioTracks = {0, 0, NULL};

// GLOBAL: MW2 0x100aa28c
MechS32 g_cdAudioInitialized = 0;

// GLOBAL: MW2 0x100beca8
undefined4 g_unk0x100beca8;

// GLOBAL: MW2 0x100becac
undefined4 g_unk0x100becac;

// GLOBAL: MW2 0x100becb0
CdAudioPosition g_pausedCdAudioPosition;

// GLOBAL: MW2 0x100becc0
MechS32 g_cdStatus;

// Picks the auxiliary audio device the CD volume goes through: the CD audio output, or failing
// that the first auxiliary input.
// Stack-slot permutation: numDevs, device and found.
// FUNCTION: MW2 0x1005a7a0
MechS32 GetCdAudioAuxDevice(void)
{
	MechS32 numDevs;
	MechS32 device;
	MechS32 i;
	AUXCAPS caps;
	BOOL found;

	found = FALSE;
	device = -1;
	numDevs = auxGetNumDevs();
	if (numDevs > 0) {
		for (i = 0; i < numDevs && !found; i++) {
			if (auxGetDevCaps(i, &caps, sizeof(caps))) {
				return -1;
			}

			if (caps.wTechnology == AUXCAPS_CDAUDIO) {
				found = TRUE;
				device = i;
			}
		}

		if (!found) {
			for (i = 0; i < numDevs && !found; i++) {
				if (auxGetDevCaps(i, &caps, sizeof(caps))) {
					return -1;
				}

				if (caps.wTechnology == AUXCAPS_AUXIN) {
					found = TRUE;
					device = i;
				}
			}
		}
	}

	return device;
}

// Opens the CD audio device of the drive holding the game disc, in TMSF time format.
// Stack-slot permutation: every local.
// FUNCTION: MW2 0x1005a8b5
MechS32 InitCdAudio(void)
{
	MechS32 cdNumber;
	MCI_SET_PARMS setParms;
	MCIERROR error;
	DWORD quantity;
	MCI_SYSINFO_PARMS sysinfoParms;
	MCI_OPEN_PARMS openParms;
	char name[32];

	cdNumber = GetGameCdNumber();
	if (cdNumber == 0) {
		return 1;
	}

	sysinfoParms.wDeviceType = MCI_DEVTYPE_CD_AUDIO;
	sysinfoParms.lpstrReturn = (LPSTR) &quantity;
	sysinfoParms.dwRetSize = sizeof(quantity);
	error = mciSendCommand(0, MCI_SYSINFO, MCI_SYSINFO_QUANTITY, (DWORD) &sysinfoParms);
	if (error) {
		return 1;
	}

	if ((MechS32) quantity < cdNumber) {
		return 1;
	}

	sysinfoParms.dwNumber = cdNumber;
	sysinfoParms.lpstrReturn = name;
	sysinfoParms.dwRetSize = sizeof(name);
	error = mciSendCommand(0, MCI_SYSINFO, MCI_SYSINFO_NAME, (DWORD) &sysinfoParms);
	if (error) {
		mciGetErrorString(error, name, sizeof(name));
		return 1;
	}

	openParms.lpstrDeviceType = name;
	error = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE, (DWORD) &openParms);
	if (error) {
		return 1;
	}

	g_cdAudioDevice = openParms.wDeviceID;
	setParms.dwTimeFormat = MCI_FORMAT_TMSF;
	if (mciSendCommand(g_cdAudioDevice, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD) &setParms)) {
		CloseCdAudio();
		return 1;
	}

	g_cdAudioAuxDevice = GetCdAudioAuxDevice();
	return 0;
}

// FUNCTION: MW2 0x1005aa0a
MechS32 CloseCdAudio(void)
{
	MCI_GENERIC_PARMS parms;

	mciSendCommand(g_cdAudioDevice, MCI_CLOSE, 0, (DWORD) &parms);
	g_cdAudioDevice = (MCIDEVICEID) -1;
	return 0;
}

// FUNCTION: MW2 0x1005aa40
void PauseCdAudio(void)
{
	MCI_GENERIC_PARMS parms;

	mciSendCommand(g_cdAudioDevice, MCI_PAUSE, 0, (DWORD) &parms);
	return;
}

// FUNCTION: MW2 0x1005aa6a
void ResumeCdAudio(void)
{
	MCI_GENERIC_PARMS parms;

	mciSendCommand(g_cdAudioDevice, MCI_RESUME, 0, (DWORD) &parms);
	return;
}

// FUNCTION: MW2 0x1005aa94
void StopCdAudio(void)
{
	MCI_GENERIC_PARMS parms;

	mciSendCommand(g_cdAudioDevice, MCI_STOP, 0, (DWORD) &parms);
	return;
}

// Plays from p_from, up to p_to unless it is 0 (TMSF positions).
// FUNCTION: MW2 0x1005aabe
void PlayCdAudio(MechU32 p_from, MechU32 p_to)
{
	MCI_PLAY_PARMS parms;
	DWORD flags;

	flags = MCI_FROM;
	parms.dwFrom = p_from;
	if (p_to) {
		flags |= MCI_TO;
		parms.dwTo = p_to;
	}

	mciSendCommand(g_cdAudioDevice, MCI_PLAY, flags, (DWORD) &parms);
	return;
}

// FUNCTION: MW2 0x1005ab0b
MechS32 StartCdAudio(void)
{
	g_unk0x100beca8 = 0;
	g_unk0x100becac = 0;
	if (InitCdAudio()) {
		return 0;
	}

	g_cdAudioInitialized = 1;
	g_cdStatus = GetCdStatus();
	switch (g_cdStatus) {
	case c_cdStatusOpen:
		g_cdAudioInitialized = 0;
		return 0;
		break;
	case c_cdStatusStopped:
		GetCdAudioTracks(&g_cdAudioTracks);
		break;
	case c_cdStatusPlaying:
		GetCdAudioTracks(&g_cdAudioTracks);
		break;
	case c_cdStatusPaused:
		GetCdAudioTracks(&g_cdAudioTracks);
		GetCdAudioPosition(&g_pausedCdAudioPosition);
		break;
	}

	SetCdAudioVolume(g_soundConfig.m_midiVolume);
	return 1;
}

// FUNCTION: MW2 0x1005abff
void DeInitCdAudio(void)
{
	if (g_cdAudioInitialized) {
		FreeCdAudioTracks(&g_cdAudioTracks);
		CloseCdAudio();
		g_cdAudioInitialized = 0;
	}
}

// FUNCTION: MW2 0x1005ac33
MechS32 GetCdStatus(void)
{
	MCI_STATUS_PARMS parms;

	if (g_cdAudioInitialized) {
		parms.dwItem = MCI_STATUS_MODE;
		if (mciSendCommand(g_cdAudioDevice, MCI_STATUS, MCI_STATUS_ITEM, (DWORD) &parms)) {
			DeInitCdAudio();
			return c_cdStatusError;
		}

		switch (parms.dwReturn) {
		case MCI_MODE_PAUSE:
			return c_cdStatusPaused;
		case MCI_MODE_PLAY:
			return c_cdStatusPlaying;
		case MCI_MODE_STOP:
			return c_cdStatusStopped;
		case MCI_MODE_OPEN:
			return c_cdStatusOpen;
		default:
			return c_cdStatusError;
		}
	}
	else {
		return c_cdStatusError;
	}
}

// FUNCTION: MW2 0x1005ad0a
MechS32 RefreshCdStatus(void)
{
	return g_cdStatus = GetCdStatus();
}

// FUNCTION: MW2 0x1005ad29
void OpenCdDoor(void)
{
	MCI_SET_PARMS parms;

	if (mciSendCommand(g_cdAudioDevice, MCI_SET, MCI_SET_DOOR_OPEN, (DWORD) &parms)) {
		DeInitCdAudio();
	}
}

// FUNCTION: MW2 0x1005ad5e
void CdAudioTogglePaused(void)
{
	if (g_cdAudioInitialized) {
		switch (GetCdStatus()) {
		case c_cdStatusOpen:
			break;
		case c_cdStatusStopped:
			PlayCdAudio(g_cdAudioTracks.m_trackPositions[0], 0);
			break;
		case c_cdStatusPlaying:
			UpdateCdAudioPosition(&g_pausedCdAudioPosition);
			PauseCdAudio();
			break;
		case c_cdStatusPaused:
			ResumeCdAudio();
			break;
		}
	}
}

// FUNCTION: MW2 0x1005adef
void ContinueCdAudio(void)
{
	if (g_cdAudioInitialized) {
		switch (GetCdStatus()) {
		case c_cdStatusOpen:
			break;
		case c_cdStatusStopped:
			PlayCdAudio(g_cdAudioTracks.m_trackPositions[0], 0);
			break;
		case c_cdStatusPlaying:
			UpdateCdAudioPosition(&g_pausedCdAudioPosition);
			break;
		case c_cdStatusPaused:
			ResumeCdAudio();
			break;
		}
	}
}

// Stops the disc, retrying until the drive reports it stopped.
// FUNCTION: MW2 0x1005ae7b
void StopCdAudioAndWait(void)
{
	MechS32 status;

	if (g_cdAudioInitialized) {
		status = c_cdStatusError;
		while (status != c_cdStatusStopped) {
			StopCdAudio();
			status = GetCdStatus();
		}
	}
}

// FUNCTION: MW2 0x1005aeb9
void PreviousCdTrack(void)
{
	if (g_cdAudioInitialized) {
		SkipCdTracks(&g_cdAudioTracks, -1);
	}
}

// FUNCTION: MW2 0x1005aee0
void NextCdTrack(void)
{
	if (g_cdAudioInitialized) {
		SkipCdTracks(&g_cdAudioTracks, 1);
	}
}

// FUNCTION: MW2 0x1005af07
void UpdateCdAudioPosition(CdAudioPosition* p_position)
{
	if (g_cdAudioInitialized) {
		switch (GetCdStatus()) {
		case c_cdStatusOpen:
		case c_cdStatusStopped:
			p_position->m_track = 0;
			p_position->m_minute = 0;
			p_position->m_second = 0;
			p_position->m_frame = 0;
			break;
		case c_cdStatusPlaying:
			GetCdAudioPosition(p_position);
			break;
		case c_cdStatusPaused:
			*p_position = g_pausedCdAudioPosition;
			break;
		}
	}
}

// FUNCTION: MW2 0x1005afbb
undefined4 FUN_1005afbb(void)
{
	return g_unk0x100becac;
}

// FUNCTION: MW2 0x1005afd0
void FUN_1005afd0(undefined4 p_unk0x00)
{
	g_unk0x100becac = p_unk0x00;
}

// FUNCTION: MW2 0x1005afe3
undefined4 FUN_1005afe3(void)
{
	return g_unk0x100beca8;
}

// FUNCTION: MW2 0x1005aff8
void FUN_1005aff8(undefined4 p_unk0x00)
{
	g_unk0x100beca8 = p_unk0x00;
}

// Tracks the drive's state: frees the table of contents when the tray opens, and rereads it when
// a disc goes in (beeping on each failed attempt, and exiting after three). Returns whether the
// state changed.
// Stack-slot permutation: status and i.
// FUNCTION: MW2 0x1005b00b
MechS32 PollCdDrive(void)
{
	MechS32 status;
	MechS32 i;

	if (g_cdAudioInitialized && (status = GetCdStatus()) != g_cdStatus) {
		switch (status) {
		case c_cdStatusOpen:
			FreeCdAudioTracks(&g_cdAudioTracks);
			break;
		case c_cdStatusStopped:
			if (g_cdStatus == c_cdStatusOpen) {
				for (i = 0; i < 3 && GetCdAudioTracks(&g_cdAudioTracks); i++) {
					_putch(7);
				}

				if (i == 3) {
					exit(0);
				}
			}
			break;
		case c_cdStatusPlaying:
			break;
		case c_cdStatusPaused:
			break;
		}

		g_cdStatus = status;
		return 1;
	}

	return 0;
}

// Skips p_step tracks from the current one.
// Stack-slot permutation: status and track.
// FUNCTION: MW2 0x1005b10b
void SkipCdTracks(CdAudioTracks* p_tracks, MechS32 p_step)
{
	MechS32 status;
	MechS32 track;
	CdAudioPosition position;

	if (g_cdAudioInitialized) {
		status = GetCdStatus();
		switch (status) {
		case c_cdStatusOpen:
		case c_cdStatusStopped:
			break;
		case c_cdStatusPlaying:
		case c_cdStatusPaused:
			UpdateCdAudioPosition(&position);
			track = position.m_track + p_step;
			if (track >= p_tracks->m_firstTrack && track <= p_tracks->m_numberOfTracks) {
				switch (status) {
				case c_cdStatusPlaying:
					StopCdAudio();
					PlayCdAudio(g_cdAudioTracks.m_trackPositions[track - g_cdAudioTracks.m_firstTrack], 0);
					break;
				case c_cdStatusPaused:
					PlayCdAudio(g_cdAudioTracks.m_trackPositions[track - g_cdAudioTracks.m_firstTrack], 0);
					StopCdAudio();
					g_pausedCdAudioPosition.m_minute = 0;
					g_pausedCdAudioPosition.m_second = 0;
					g_pausedCdAudioPosition.m_track = track;
					break;
				}
			}
		}
	}
}

// Plays track p_track to its end.
// Stack-slot permutation: tracks and status.
// FUNCTION: MW2 0x1005b22f
void PlayCdTrack(MechS32 p_track)
{
	CdAudioPosition position;
	CdAudioTracks* tracks;
	MechS32 status;

	if (g_cdAudioInitialized) {
		tracks = &g_cdAudioTracks;
		status = GetCdStatus();
		switch (status) {
		case c_cdStatusOpen:
			break;
		case c_cdStatusStopped:
		case c_cdStatusPlaying:
		case c_cdStatusPaused:
			UpdateCdAudioPosition(&position);
			if (p_track >= tracks->m_firstTrack && p_track <= tracks->m_numberOfTracks) {
				switch (status) {
				case c_cdStatusPlaying:
					StopCdAudio();
					PlayCdAudio(
						g_cdAudioTracks.m_trackPositions[p_track - g_cdAudioTracks.m_firstTrack],
						g_cdAudioTracks.m_trackPositions[p_track + 1 - g_cdAudioTracks.m_firstTrack]
					);
					break;
				case c_cdStatusPaused:
					PlayCdAudio(
						g_cdAudioTracks.m_trackPositions[p_track - g_cdAudioTracks.m_firstTrack],
						g_cdAudioTracks.m_trackPositions[p_track + 1 - g_cdAudioTracks.m_firstTrack]
					);
					g_pausedCdAudioPosition.m_track = p_track;
					g_pausedCdAudioPosition.m_minute = 0;
					g_pausedCdAudioPosition.m_second = 0;
					break;
				default:
					PlayCdAudio(
						g_cdAudioTracks.m_trackPositions[p_track - g_cdAudioTracks.m_firstTrack],
						g_cdAudioTracks.m_trackPositions[p_track + 1 - g_cdAudioTracks.m_firstTrack]
					);
					break;
				}
			}
		}
	}
}

// FUNCTION: MW2 0x1005b3a0
void PlayNewCdAudio(MechU32 p_from, MechU32 p_to)
{
	CdAudioPosition position;
	MechS32 status;

	if (g_cdAudioInitialized) {
		status = GetCdStatus();
		switch (status) {
		case c_cdStatusOpen:
			break;
		case c_cdStatusStopped:
		case c_cdStatusPlaying:
		case c_cdStatusPaused:
			UpdateCdAudioPosition(&position);
			if (p_to > p_from) {
				switch (status) {
				case c_cdStatusPlaying:
					StopCdAudio();
					PlayCdAudio(p_from, p_to);
					break;
				case c_cdStatusPaused:
					PlayCdAudio(p_from, p_to);
					g_pausedCdAudioPosition.m_track = 0;
					g_pausedCdAudioPosition.m_minute = 0;
					g_pausedCdAudioPosition.m_second = 0;
					break;
				default:
					PlayCdAudio(p_from, p_to);
					break;
				}
			}
		}
	}
}

// Reads the disc's table of contents into p_tracks. Returns 0 on success.
// Stack-slot permutation: count and track.
// FUNCTION: MW2 0x1005b49e
MechS32 GetCdAudioTracks(CdAudioTracks* p_tracks)
{
	MechS32 count;
	MechS32 track;
	MCI_STATUS_PARMS parms;

	if (g_cdAudioInitialized == 0) {
		return 1;
	}

	parms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	if (mciSendCommand(g_cdAudioDevice, MCI_STATUS, MCI_STATUS_ITEM, (DWORD) &parms)) {
		DeInitCdAudio();
		return 0;
	}

	count = parms.dwReturn;
	p_tracks->m_firstTrack = 1;
	p_tracks->m_numberOfTracks = count;
	p_tracks->m_trackPositions = MechHeapAlloc(g_primaryHeap, (count + 1) * 4);
	if (p_tracks->m_trackPositions == NULL) {
		g_cdAudioInitialized = 0;
		return 1;
	}

	for (track = p_tracks->m_firstTrack; track <= p_tracks->m_numberOfTracks; track++) {
		parms.dwItem = MCI_STATUS_POSITION;
		parms.dwTrack = track;
		if (mciSendCommand(g_cdAudioDevice, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD) &parms)) {
			DeInitCdAudio();
			return 0;
		}

		p_tracks->m_trackPositions[track - p_tracks->m_firstTrack] = parms.dwReturn;
	}

	p_tracks->m_trackPositions[track - p_tracks->m_firstTrack] = MCI_MAKE_TMSF(p_tracks->m_numberOfTracks + 1, 0, 0, 0);
	return 0;
}

// FUNCTION: MW2 0x1005b5e6
void FreeCdAudioTracks(CdAudioTracks* p_tracks)
{
	if (p_tracks->m_trackPositions) {
		MechHeapFree(g_primaryHeap, p_tracks->m_trackPositions);
		p_tracks->m_trackPositions = NULL;
	}
}

// FUNCTION: MW2 0x1005b61d
void GetCdAudioPosition(CdAudioPosition* p_position)
{
	MCI_STATUS_PARMS parms;

	if (g_cdAudioInitialized) {
		parms.dwItem = MCI_STATUS_POSITION;
		if (mciSendCommand(g_cdAudioDevice, MCI_STATUS, MCI_STATUS_ITEM, (DWORD) &parms) == 0) {
			p_position->m_track = MCI_TMSF_TRACK(parms.dwReturn);
			p_position->m_minute = MCI_TMSF_MINUTE(parms.dwReturn);
			p_position->m_second = MCI_TMSF_SECOND(parms.dwReturn);
			p_position->m_frame = MCI_TMSF_FRAME(parms.dwReturn);
		}
	}
}

// FUNCTION: MW2 0x1005b696
MechS32 IsCdAudioInitialized(void)
{
	return g_cdAudioInitialized;
}

// Returns whether p_track is on the disc.
// FUNCTION: MW2 0x1005b6ab
MechS32 IsCdTrackOnDisc(MechS32 p_track)
{
	if (p_track >= g_cdAudioTracks.m_firstTrack && p_track <= g_cdAudioTracks.m_numberOfTracks) {
		return 1;
	}

	return 0;
}

// Returns the left channel's volume of the CD's auxiliary device, or -1.
// FUNCTION: MW2 0x1005b6e5
MechS32 GetCdAudioVolume(void)
{
	DWORD volume;

	if (g_cdAudioAuxDevice != -1 && auxGetVolume(g_cdAudioAuxDevice, &volume) == 0) {
		return volume & 0xffff;
	}
	else {
		return -1;
	}
}

// Sets both channels of the CD's auxiliary device to p_volume (at most 0xffff).
// FUNCTION: MW2 0x1005b734
MechS32 SetCdAudioVolume(MechS32 p_volume)
{
	p_volume = min(p_volume, 0xffff);
	p_volume &= 0xffff;
	p_volume |= p_volume << 16;
	if (g_cdAudioAuxDevice != -1 && auxSetVolume(g_cdAudioAuxDevice, p_volume) == 0) {
		return 1;
	}
	else {
		return 0;
	}
}

// FUNCTION: MW2 0x1005b7a0
void ApplyCdAudioVolume(void)
{
	SetCdAudioVolume(g_soundConfig.m_midiVolume);
}
