#ifndef CDAUDIO_H
#define CDAUDIO_H

#include "decomp.h"
#include "types.h"

// The mission's music, implemented on the Rust side (src/sim/cd_audio.rs). The original played
// the game disc's audio tracks through MCI; the tracks are music files now, but the interface is
// still the original's view of a CD drive.

// The state of the CD drive, as GetCdStatus reports it.
enum {
	c_cdStatusOpen = 1,
	c_cdStatusStopped = 2,
	c_cdStatusPlaying = 3,
	c_cdStatusPaused = 4,
	c_cdStatusError = 5
};

#ifdef __cplusplus
extern "C"
{
#endif

	// Finds the tracks and sets the music volume. Returns 0 when there is no music.
	MechS32 StartCdAudio(void);
	void DeInitCdAudio(void);
	MechS32 IsCdAudioInitialized(void);
	MechS32 IsCdTrackOnDisc(MechS32 p_track);

	MechS32 GetCdStatus(void);
	// GetCdStatus, remembered for PollCdDrive
	MechS32 RefreshCdStatus(void);
	// Returns whether the status changed since it was last remembered, and remembers it
	MechS32 PollCdDrive(void);

	// Plays track p_track to its end.
	void PlayCdTrack(MechS32 p_track);
	void CdAudioTogglePaused(void);
	void StopCdAudioAndWait(void);
	// Sets the music volume from the sound configuration
	void ApplyCdAudioVolume(void);

#ifdef __cplusplus
}
#endif

#endif // CDAUDIO_H
