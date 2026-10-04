/* The mission's sound: the digital effects, the MIDI or CD music the mission's "MUS" resource
   names, and the warning tone when an AI player engages the local player. */
#include "audio.h"

#include "cdaudio.h"
#include "clock.h"
#include "config.h"
#include "decomp.h"
#include "loadres.h"
#include "midi.h"
#include "mss.h"
#include "mw2prj.h"
#include "network.h"
#include "players.h"
#include "simmain.h"
#include "soundfx.h"
#include "soundlimits.h"
#include "speech.h"
#include "types.h"
#include "world.h"

#include <stdio.h>
#include <windows.h>

// The CD track the mission plays, or -1.
// GLOBAL: MW2 0x100a1490
MechS32 g_cdTrack = -1;

// The MIDI sequence the mission plays, or -1.
// GLOBAL: MW2 0x100a1494
MechS32 g_midiSequence = -1;

// GLOBAL: MW2 0x100a1498
SoundConfig g_soundConfig = {0x10000, 0x10000, 0x10000, 0x10000, 11, 1, 1, 1, 1, 1, 9, "mcga.dll"};

// GLOBAL: MW2 0x100a14d4
SoundConfig* g_mw2SndCfgData = NULL;

// GLOBAL: MW2 0x100a14d8
MechS32 g_audioPaused = 0;

// GLOBAL: MW2 0x100a14dc
MechS32 g_musicStarted = 0;

// GLOBAL: MW2 0x100bcda8
static PCMWAVEFORMAT g_waveFormat;

// GLOBAL: MW2 0x10179e80
MechS32 g_nextEngageCheck;

// The in-mission sound menu's settings: 0 the effects volume, 1 the voice volume, 2 the music
// volume, 3 the first word of the sound configuration, 4 whether speech is on.

// Returns setting p_setting. Asking for the music volume turns the CD music on.
// FUNCTION: MW2 0x10006760
MechS32 GetSoundSetting(MechS32 p_setting)
{
	MechS32 value;

	value = 0;
	if (p_setting >= 0 && p_setting < 5) {
		switch (p_setting) {
		case 3:
			value = g_soundConfig.m_unk0x00;
			break;
		case 0:
			value = g_soundConfig.m_effectsVolume;
			break;
		case 1:
			value = g_soundConfig.m_voiceVolume;
			break;
		case 2:
			value = g_soundConfig.m_midiVolume;
			g_soundConfig.m_simFlags |= 8;
			if (!g_isNetworkGame) {
				StartMissionMusic();
				PauseMusic();
			}
			break;
		case 4:
			value = (g_soundConfig.m_simFlags & 2) ? 1 : 0;
			break;
		default:
			break;
		}
	}

	return value;
}

// Sets setting p_setting to p_value, and lets the sound system that uses it know.
// FUNCTION: MW2 0x10006845
void PreviewSoundSetting(MechS32 p_setting, MechS32 p_value)
{
	MechS32 old;
	void (*notify)(void);

	old = 0;
	notify = NULL;
	if (p_setting >= 0 && p_setting < 5) {
		if (p_setting != 2) {
			PauseMusic();
		}

		switch (p_setting) {
		case 3:
			old = g_soundConfig.m_unk0x00;
			g_soundConfig.m_unk0x00 = p_value;
			if (g_soundConfig.m_unk0x00 != old) {
				notify = NULL;
			}
			break;
		case 0:
			old = g_soundConfig.m_effectsVolume;
			g_soundConfig.m_effectsVolume = p_value;
			if (g_soundConfig.m_effectsVolume != old) {
				notify = PlayEffectsVolumeTest;
			}
			break;
		case 1:
			old = g_soundConfig.m_voiceVolume;
			g_soundConfig.m_voiceVolume = p_value;
			if (p_value) {
				g_soundConfig.m_simFlags |= 2;
			}
			else {
				g_soundConfig.m_simFlags &= ~2;
			}

			if (g_soundConfig.m_voiceVolume != old) {
				notify = PlayVoiceVolumeTest;
			}
			break;
		case 2:
			old = g_soundConfig.m_midiVolume;
			g_soundConfig.m_midiVolume = p_value;
			if (g_soundConfig.m_midiVolume != old) {
				if (IsCdAudioInitialized()) {
					notify = ApplyCdAudioVolume;
				}
				else {
					notify = ApplyMidiVolume;
				}

				if (!old) {
					StartMissionMusic();
				}
			}

			ResumeMusic();
			break;
		case 4:
			break;
		default:
			break;
		}

		if (notify) {
			notify();
		}
	}
}

// Sets setting p_setting to p_value, in the settings MW2SND.CFG saves too.
// FUNCTION: MW2 0x100069c9
void SetSoundSetting(MechS32 p_setting, MechS32 p_value)
{
	if (p_setting >= 0 && p_setting < 5) {
		switch (p_setting) {
		case 3:
			g_mw2SndCfgData->m_unk0x00 = g_soundConfig.m_unk0x00 = p_value;
			break;
		case 0:
			g_mw2SndCfgData->m_effectsVolume = g_soundConfig.m_effectsVolume = p_value;
			break;
		case 1:
			g_mw2SndCfgData->m_voiceVolume = g_soundConfig.m_voiceVolume = p_value;
			if (p_value) {
				g_soundConfig.m_simFlags |= 2;
			}
			else {
				g_soundConfig.m_simFlags &= ~2;
			}
			break;
		case 2:
			g_mw2SndCfgData->m_midiVolume = g_soundConfig.m_midiVolume = p_value;
			if (p_value) {
				g_mw2SndCfgData->m_simFlags |= 8;
				g_mw2SndCfgData->m_simFlags |= 4;
				if (!g_isNetworkGame) {
					PauseMusic();
				}
			}
			else {
				g_mw2SndCfgData->m_simFlags &= ~8;
				g_mw2SndCfgData->m_simFlags &= ~4;
				StopMusic();
			}

			g_soundConfig.m_simFlags = g_mw2SndCfgData->m_simFlags;
			break;
		case 4:
			if (p_value) {
				g_mw2SndCfgData->m_simFlags |= 2;
			}
			else {
				g_mw2SndCfgData->m_simFlags &= ~2;
			}

			g_soundConfig.m_simFlags = g_mw2SndCfgData->m_simFlags;
			break;
		default:
			break;
		}
	}
}

// Restores setting p_setting from the settings MW2SND.CFG saves.
// FUNCTION: MW2 0x10006b3a
void RestoreSoundSetting(MechS32 p_setting)
{
	if (p_setting >= 0 && p_setting < 5) {
		switch (p_setting) {
		case 3:
			g_soundConfig.m_unk0x00 = g_mw2SndCfgData->m_unk0x00;
			break;
		case 0:
			g_soundConfig.m_effectsVolume = g_mw2SndCfgData->m_effectsVolume;
			break;
		case 1:
			g_soundConfig.m_voiceVolume = g_mw2SndCfgData->m_voiceVolume;
			if (g_soundConfig.m_voiceVolume) {
				g_soundConfig.m_simFlags |= 2;
			}
			else {
				g_soundConfig.m_simFlags &= ~2;
			}
			break;
		case 2:
			g_soundConfig.m_midiVolume = g_mw2SndCfgData->m_midiVolume;
			if (g_soundConfig.m_midiVolume) {
				g_soundConfig.m_simFlags |= 8;
				g_soundConfig.m_simFlags |= 4;
				ApplyCdAudioVolume();
				if (!g_isNetworkGame) {
					PauseMusic();
				}
			}
			else {
				g_soundConfig.m_simFlags &= ~8;
				g_soundConfig.m_simFlags &= ~4;
				StopMusic();
			}
			break;
		case 4:
			g_soundConfig.m_simFlags = g_mw2SndCfgData->m_simFlags;
			break;
		default:
			break;
		}
	}
}

// FUNCTION: MW2 0x10006c5c
void StartMissionMusic(void)
{
	MechChar* music;

	if (!g_musicStarted) {
		g_musicStarted = 1;
		if (g_musicResource <= 0 ||
			(music = LoadCachedResource(0, g_musicResource, g_resourceTypeTags[c_resTagMus], 0)) == NULL) {
			return;
		}

		sscanf(music, "%d %d", &g_cdTrack, &g_midiSequence);
		UnlockCachedResource(g_musicResource, g_resourceTypeTags[c_resTagMus]);
	}

	if (!IsCdAudioInitialized() || GetCdStatus() == 1 || !IsCdTrackOnDisc(g_cdTrack)) {
		g_cdTrack = -1;
	}
	else if (g_soundConfig.m_simFlags & 8) {
		PlayCdTrack(g_cdTrack);
		if (RefreshCdStatus() != 3) {
			g_cdTrack = -1;
		}
		else {
			g_midiSequence = -1;
		}
	}

	g_midiSequence = -1;
}

// FUNCTION: MW2 0x10006d73
void PauseMusic(void)
{
	if (!g_audioPaused) {
		if (g_midiSequence != -1 && (g_soundConfig.m_simFlags & 4)) {
			PauseMidiSequences();
		}

		if (g_cdTrack != -1 && (g_soundConfig.m_simFlags & 8)) {
			CdAudioTogglePaused();
		}

		g_audioPaused = 1;
	}
}

// FUNCTION: MW2 0x10006dd3
void ResumeMusic(void)
{
	if (g_audioPaused) {
		if (g_midiSequence != -1 && (g_soundConfig.m_simFlags & 4)) {
			ResumeMidiSequences();
		}

		if (g_cdTrack != -1 && (g_soundConfig.m_simFlags & 8)) {
			CdAudioTogglePaused();
		}

		g_audioPaused = 0;
	}
}

// FUNCTION: MW2 0x10006e33
void StopMusic(void)
{
	if (g_cdTrack != -1) {
		StopCdAudioAndWait();
	}

	if (g_midiSequence != -1) {
		StopMidiSequences();
	}
}

// FUNCTION: MW2 0x10006e62
void LoopCdMusic(void)
{
	if (!g_audioPaused && (g_soundConfig.m_simFlags & 8)) {
		if (PollCdDrive() && g_cdTrack != -1) {
			PlayCdTrack(g_cdTrack);
			RefreshCdStatus();
		}
	}
}

// FUNCTION: MW2 0x10006eb4
MechS32 FirstAudio(void)
{
	InitializeDigitalAudio(8);
	InitializeMidi();
	StartCdAudio();
	InitSoundInfo();
	g_nextEngageCheck = g_currentClock + 0x389;
	return 1;
}

// Serves the samples and, every five seconds, sounds a warning while an AI player within
// 150000 attacks the local player.
// Operand order: g_nextEngageCheck <= g_currentClock and i < g_playerCount load the other
// operand first in the original.
// FUNCTION: MW2 0x10006ef1
void DoAudio(void)
{
	Player* player;
	MechS32 i;

	AIL_serve();
	ServeSamples();
	if (g_nextEngageCheck <= g_currentClock) {
		g_nextEngageCheck = g_currentClock + 0x389;
		if (!(g_players[g_localPlayerId]->m_flags & 0x16)) {
			for (i = 0; i < g_playerCount; i++) {
				player = g_players[i];
				if (player->m_index == g_localPlayerId) {
					continue;
				}

				if ((player->m_ai.m_goal & 0xff) == g_localPlayerId &&
					(player->m_ai.m_state == 2 || player->m_ai.m_state == 3) &&
					player->m_targetInfo.m_range <= 150000) {
					PlaySoundEffect(0x100, 100, 0x40, 5, 0x32);
					break;
				}
			}
		}
	}
}

// FUNCTION: MW2 0x10006ffa
void ShutdownAudio(void)
{
	StopMusic();
	DeInitCdAudio();
	ShutdownMidi();
	ShutdownDigitalAudio();
	SaveSndCfg("mw2snd.cfg", g_mw2SndCfgData);
	HeapFree(g_primaryHeap, HEAP_NO_SERIALIZE, g_mw2SndCfgData);
}

// FUNCTION: MW2 0x10007040
void PauseAudio(void)
{
	StopSamples(0);
	StopSpeech();
	PauseMusic();
	MuteEngineNote();
}

// FUNCTION: MW2 0x10007064
void ResumeAudio(void)
{
	ResumeMusic();
	ResumeSpeech();
}

// Opens the digital driver at 11025 Hz, 8-bit stereo.
// FUNCTION: MW2 0x10007079
HDIGDRIVER OpenDigitalDriver(void)
{
	HDIGDRIVER driver;

	g_waveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
	g_waveFormat.wf.nChannels = 2;
	g_waveFormat.wf.nSamplesPerSec = 11025;
	g_waveFormat.wf.nAvgBytesPerSec = 22050;
	g_waveFormat.wf.nBlockAlign = 2;
	g_waveFormat.wBitsPerSample = 8;
	if (AIL_waveOutOpen(&driver, NULL, 0, (LPWAVEFORMAT) &g_waveFormat)) {
		return NULL;
	}
	else {
		return driver;
	}
}

// Opens the MIDI mapper, or MIDI device 0.
// FUNCTION: MW2 0x100070ee
HMDIDRIVER OpenMidiDriver(void)
{
	HMDIDRIVER driver;

	if (AIL_midiOutOpen(&driver, NULL, -1) && AIL_midiOutOpen(&driver, NULL, 0)) {
		return NULL;
	}
	else {
		return driver;
	}
}
