/* The digital sound effects (AudioEngine): Miles samples streamed from the sound resources,
   positioned sounds panned by their bearing from the eyepoint, and the looping ambient sounds. */
#include "soundfx.h"

#include "ambientsound.h"
#include "audio.h"
#include "audioengine.h"
#include "clock.h"
#include "decomp.h"
#include "environment.h"
#include "fixedmul.h"
#include "fixedtrig.h"
#include "loadres.h"
#include "mss.h"
#include "mw2prj.h"
#include "object.h"
#include "polydraw.h"
#include "random.h"
#include "simmain.h"
#include "sndunpack.h"
#include "soundinfo.h"
#include "speech.h"
#include "targeting.h"
#include "types.h"

#include <string.h>

// GLOBAL: MW2 0x100ba620
MechS32 g_temperature = 25;

// Set on a planet without a breathable atmosphere: the local player can't eject (the cockpit says
// so), a life support hit kills, and a mech whose pilot ejects counts as lost.
// GLOBAL: MW2 0x100ba624
MechS32 g_hostileAtmosphere = 0;

// GLOBAL: MW2 0x100ba628
MechS32 g_sampleRates[10] = {9922, 10143, 10364, 10584, 10804, 11025, 11246, 11466, 11686, 11907};

// GLOBAL: MW2 0x100ba650
AudioEngine* g_audioEngine = NULL;

// GLOBAL: MW2 0x100ba654
MechChar g_sampleTag[] = "SFLX";

// GLOBAL: MW2 0x10174930
SoundInfo g_soundInfo[1200];

// FUNCTION: MW2 0x1007d9d0
MechS32 InitializeDigitalAudio(MechU32 p_numSamples)
{
	MechU32 i;

	if (g_audioEngine) {
		return -1;
	}

	g_audioEngine = MechHeapAlloc(g_primaryHeap, sizeof(AudioEngine));
	if (!g_audioEngine) {
		return -3;
	}

	memset(g_audioEngine, 0, sizeof(AudioEngine));
	g_audioEngine->m_driver = OpenDigitalDriver();
	if (!g_audioEngine->m_driver) {
		MechHeapFree(g_primaryHeap, g_audioEngine);
		g_audioEngine = NULL;
		return -4;
	}

	if (p_numSamples <= 16) {
		g_audioEngine->m_numSamples = p_numSamples;
	}
	else {
		g_audioEngine->m_numSamples = 16;
	}

	for (i = 0; i < g_audioEngine->m_numSamples; i++) {
		g_audioEngine->m_samples[i] = AIL_allocate_sample_handle(g_audioEngine->m_driver);
		if (!g_audioEngine->m_samples[i]) {
			g_audioEngine->m_numSamples = i;
			break;
		}
		else {
			g_audioEngine->m_buffers[i][0] = MechHeapAlloc(g_primaryHeap, 0x4000);
			if (!g_audioEngine->m_buffers[i][0]) {
				g_audioEngine->m_numSamples = i;
				break;
			}
		}
	}

	for (i = 0; i < g_audioEngine->m_numSamples; i++) {
		g_audioEngine->m_playing[i] = 0;
		g_audioEngine->m_startTimes[i] = -1;
		g_audioEngine->m_bearings[i] = 0;
		g_audioEngine->m_pans[i] = 64;
	}

	return 1;
}

// FUNCTION: MW2 0x1007dbce
void ShutdownDigitalAudio(void)
{
	MechU32 i;

	if (!g_audioEngine) {
		return;
	}

	for (i = 0; i < g_audioEngine->m_numSamples; i++) {
		if (g_audioEngine->m_playing[i]) {
			AIL_end_sample(g_audioEngine->m_samples[i]);
			g_audioEngine->m_playing[i] = 0;
		}

		MechHeapFree(g_primaryHeap, g_audioEngine->m_buffers[i][0]);
	}

	MechHeapFree(g_primaryHeap, g_audioEngine);
	g_audioEngine = NULL;
}

// Loads the next part of each playing sample into whichever half of its buffer Miles has
// finished with.
// Stack slots: i, size and buffer are permuted. Operand order: the index of m_buffers[i][buffer],
// m_remaining[i] <= m_chunkFrames[i] and the product for size.
// FUNCTION: MW2 0x1007dc89
void ServeSamples(void)
{
	MechU32 i;
	MechU32 size;
	MechS32 buffer;

	if (!g_audioEngine) {
		return;
	}

	for (i = 0; i < g_audioEngine->m_numSamples; i++) {
		if (!g_audioEngine->m_playing[i] || !g_audioEngine->m_data[i]) {
			continue;
		}

		if (g_audioEngine->m_startTimes[i] > g_currentClock) {
			continue;
		}

		buffer = AIL_sample_buffer_ready(g_audioEngine->m_samples[i]);
		if (buffer == -1) {
			continue;
		}

		if (g_audioEngine->m_pans[i] == -1) {
			AIL_set_sample_pan(g_audioEngine->m_samples[i], CalculateSamplePan(g_audioEngine->m_bearings[i]));
		}
		else {
			AIL_set_sample_pan(g_audioEngine->m_samples[i], g_audioEngine->m_pans[i]);
		}

		AIL_set_sample_volume(g_audioEngine->m_samples[i], g_audioEngine->m_volumes[i]);

		if (!g_audioEngine->m_remaining[i]) {
			AIL_load_sample_buffer(g_audioEngine->m_samples[i], buffer, g_audioEngine->m_buffers[i][buffer], 0);
			continue;
		}

		if (g_audioEngine->m_remaining[i] <= g_audioEngine->m_chunkFrames[i]) {
			g_audioEngine->m_data[i] = DecodeSoundFrames(
				g_audioEngine->m_data[i],
				g_audioEngine->m_buffers[i][buffer],
				g_audioEngine->m_remaining[i],
				g_audioEngine->m_frameSizes[i],
				&g_audioEngine->m_decodeStates[i]
			);
			size = g_audioEngine->m_frameSizes[i] * g_audioEngine->m_remaining[i];
			g_audioEngine->m_remaining[i] = 0;
		}
		else {
			g_audioEngine->m_data[i] = DecodeSoundFrames(
				g_audioEngine->m_data[i],
				g_audioEngine->m_buffers[i][buffer],
				g_audioEngine->m_chunkFrames[i],
				g_audioEngine->m_frameSizes[i],
				&g_audioEngine->m_decodeStates[i]
			);
			size = g_audioEngine->m_bufferSizes[i];
			g_audioEngine->m_remaining[i] -= g_audioEngine->m_chunkFrames[i];
		}

		AIL_load_sample_buffer(g_audioEngine->m_samples[i], buffer, g_audioEngine->m_buffers[i][buffer], size);
	}
}

// Plays a sound resource (p_id), or the sound data at p_data, after p_delay clock ticks.
// Returns the sample slot, or a negative error.
// Operand order: p_delay > g_deltaTime.
// FUNCTION: MW2 0x1007dfe7
MechS32 PlaySample(
	MechS32 p_delay,
	MechS32 p_bearing,
	MechS32 p_id,
	void* p_data,
	MechU32 p_volume,
	MechS32 p_volumeScale,
	MechS32 p_pan,
	MechS32 p_rate,
	MechS32* p_userData,
	MechU16 p_flags
)
{
	HSAMPLE sample;
	MechS32 slot;

	if (!g_audioEngine) {
		return -2;
	}

	if (p_volume <= 0) {
		return -11;
	}

	p_flags &= 0xff00;
	if (p_id < 1) {
		if (!p_data) {
			return -6;
		}

		p_flags |= 0x32;
	}
	else if (g_soundInfo[p_id].m_priority != -1) {
		p_flags |= g_soundInfo[p_id].m_priority;
	}
	else {
		p_flags |= 0x32;
	}

	slot = StartSample(p_id, p_data, p_flags, -1, p_userData);
	if (slot < 0) {
		return slot;
	}

	sample = g_audioEngine->m_samples[slot];
	AIL_set_sample_type(sample, 0, 0);

	if (p_id > 0 && g_soundInfo[p_id].m_rate != -1) {
		p_rate = g_soundInfo[p_id].m_rate;
	}
	else {
		p_rate = g_sampleRates[5];
	}

	AIL_set_sample_playback_rate(sample, p_rate);

	g_audioEngine->m_startTimes[slot] = g_currentClock;
	if (p_delay > g_deltaTime) {
		g_audioEngine->m_startTimes[slot] += p_delay;
	}

	g_audioEngine->m_pans[slot] = p_pan;
	g_audioEngine->m_bearings[slot] = p_bearing;
	p_volume = FixedMul16(p_volume, p_volumeScale);
	g_audioEngine->m_volumes[slot] = p_volume;
	g_audioEngine->m_playing[slot] = 1;

	return slot;
}

// Sets up a sample slot for a sound: p_slot, or a free one, or one playing a sound of lower
// priority. The m_flags bits: 0x100 keeps the slot, 0x200 restarts the same sound, 0x400
// plays it only once, 0x1000 loops.
// Stack slots: sample, victim, i, slot and flags are permuted. Operand order:
// m_chunkFrames[slot] > m_remaining[slot].
// FUNCTION: MW2 0x1007e1cf
MechS32 StartSample(MechS32 p_id, void* p_data, MechU16 p_flags, MechS16 p_slot, MechS32* p_userData)
{
	HSAMPLE sample;
	MechS32 victim;
	MechU32 i;
	MechS32 slot;
	MechU16 flags;

	if (p_data) {
		p_id = -1;
	}

	if (p_id > 0) {
		if (g_soundInfo[p_id].m_count != -1 && g_soundInfo[p_id].m_maxCount != -1 &&
			g_soundInfo[p_id].m_maxCount <= g_soundInfo[p_id].m_count) {
			return -10;
		}
	}
	else if (!p_data) {
		return -6;
	}

	slot = victim = -1;
	if (p_slot != -1) {
		if (g_audioEngine->m_playing[p_slot]) {
			return -8;
		}
		else {
			slot = p_slot;
		}
	}

	flags = p_flags;
	if (slot == -1) {
		for (i = 0; i < g_audioEngine->m_numSamples && slot == -1; i++) {
			if ((MechU8) (g_audioEngine->m_flags[i] >> 8)) {
				if (g_audioEngine->m_flags[i] & 0x100) {
					if (flags & 0x100) {
						return -9;
					}
					else {
						continue;
					}
				}

				if ((g_audioEngine->m_flags[i] & 0x400) && (flags & 0x400) && g_audioEngine->m_ids[i] == p_id) {
					return -10;
				}

				if ((g_audioEngine->m_flags[i] & 0x200) && (flags & 0x200) && g_audioEngine->m_ids[i] == p_id) {
					AIL_end_sample(g_audioEngine->m_samples[i]);
					g_audioEngine->m_playing[i] = 0;
					g_audioEngine->m_flags[i] = 0;
				}
			}

			if (!g_audioEngine->m_playing[i]) {
				slot = i;
			}
			else if ((MechS8) g_audioEngine->m_flags[i] < p_flags) {
				victim = i;
			}
		}

		if (slot == -1) {
			if (victim == -1) {
				return -8;
			}

			slot = victim;
			AIL_end_sample(g_audioEngine->m_samples[slot]);
			g_audioEngine->m_playing[slot] = 0;
		}
	}

	if (p_data) {
		g_audioEngine->m_data[slot] = p_data;
	}
	else {
		g_audioEngine->m_data[slot] = LoadCachedResource(0, p_id, g_resourceTypeTags[c_resTagSnds], 0);
		if (!g_audioEngine->m_data[slot]) {
			return -6;
		}
	}

	g_audioEngine->m_headers[slot] = *(SampleHeader*) g_audioEngine->m_data[slot];
	g_audioEngine->m_data[slot] += sizeof(SampleHeader);

	if (strncmp(g_audioEngine->m_headers[slot].m_tag, g_sampleTag, 4)) {
		if (p_data) {
			MechHeapFree(g_primaryHeap, p_data);
		}
		else {
			UnlockCachedResource(p_id, g_resourceTypeTags[c_resTagSnds]);
		}

		g_audioEngine->m_data[slot] = NULL;
		return -7;
	}

	g_audioEngine->m_flags[slot] = flags;
	g_audioEngine->m_ids[slot] = p_id;
	g_audioEngine->m_remaining[slot] = g_audioEngine->m_headers[slot].m_length;
	g_audioEngine->m_frameSizes[slot] = g_audioEngine->m_headers[slot].m_frameSize;
	g_audioEngine->m_bufferFrames[slot] = 0x2000 / g_audioEngine->m_frameSizes[slot];
	g_audioEngine->m_bufferSizes[slot] = g_audioEngine->m_bufferFrames[slot] * g_audioEngine->m_frameSizes[slot];
	g_audioEngine->m_buffers[slot][1] = g_audioEngine->m_buffers[slot][0] + g_audioEngine->m_bufferSizes[slot];
	g_audioEngine->m_chunkFrames[slot] = g_audioEngine->m_bufferFrames[slot];
	if (g_audioEngine->m_chunkFrames[slot] > g_audioEngine->m_remaining[slot]) {
		g_audioEngine->m_chunkFrames[slot] = g_audioEngine->m_remaining[slot];
	}

	g_audioEngine->m_decodeStates[slot] = 0;

	sample = g_audioEngine->m_samples[slot];
	AIL_init_sample(sample);

	if (p_userData == (MechS32*) -1) {
		p_userData = &g_audioEngine->m_playing[slot];
	}

	if (p_userData) {
		AIL_set_sample_user_data(sample, 0, (MECH_INTPTR) p_userData);
	}

	AIL_set_sample_user_data(sample, 1, p_id);
	AIL_set_sample_user_data(sample, 2, slot);
	AIL_set_sample_user_data(sample, 3, 0);
	AIL_set_sample_user_data(sample, 4, (MECH_INTPTR) p_data);
	AIL_register_EOS_callback(sample, SampleEosCallback);

	if (p_id > 0 && g_soundInfo[p_id].m_count != -1) {
		g_soundInfo[p_id].m_count++;
	}

	return slot;
}

// The sample's user data: 0 the flag to clear, 1 the sound, 2 the slot, 3 the sound to play
// next (-1 for none), 4 heap data to free.
// Stack slots: id, data, next, slot and done are permuted.
// FUNCTION: MW2 0x1007e80e
void AILCALLBACK SampleEosCallback(HSAMPLE p_sample)
{
	MechS32 id;
	void* data;
	MechS32 next;
	MechS32 slot;
	MechS32* done;

	id = AIL_sample_user_data(p_sample, 1);
	slot = AIL_sample_user_data(p_sample, 2);
	next = AIL_sample_user_data(p_sample, 3);

	g_audioEngine->m_playing[slot] = 0;
	if (g_soundInfo[id].m_count > 0) {
		g_soundInfo[id].m_count--;
	}

	if (id && id > 0) {
		UnlockCachedResource(id, g_resourceTypeTags[c_resTagSnds]);
	}

	data = (void*) AIL_sample_user_data(p_sample, 4);
	if (data) {
		MechHeapFree(g_primaryHeap, data);
	}

	if (next) {
		if (next != -1) {
			if (StartSample(next, NULL, g_audioEngine->m_flags[slot], slot, NULL) < 0) {
				UnlockCachedResource(next, g_resourceTypeTags[c_resTagSnds]);
				next = 0;
			}
			else {
				AIL_set_sample_user_data(p_sample, 1, next);
				if (g_audioEngine->m_flags[slot] & 0x1000) {
					AIL_set_sample_user_data(p_sample, 3, next);
				}
				else {
					AIL_set_sample_user_data(p_sample, 3, -1);
				}

				g_audioEngine->m_playing[slot] = 1;
			}
		}
		else {
			next = 0;
		}
	}

	if (!next) {
		g_audioEngine->m_ids[slot] = 0;
		g_audioEngine->m_flags[slot] = 0;
		done = (MechS32*) AIL_sample_user_data(p_sample, 0);
		*done = 0;
		// The original passed &g_speechSample as the speech's done flag, which the 32-bit store
		// above cleared; the handle is wider now.
		if (p_sample == g_speechSample) {
			g_speechSample = 0;
		}
	}
}

// FUNCTION: MW2 0x1007e9dc
void PlayEffectsVolumeTest(void)
{
	PlaySample(0, 0, 0xd2, NULL, 100, g_soundConfig.m_effectsVolume, 0x40, RandomSampleRate(), (MechS32*) -1, 0x250);
}

// FUNCTION: MW2 0x1007ea11
MechS32 PlaySoundOnce(MechS32 p_id, MechU32 p_volume, MechS32 p_pan, MechS32 p_rate)
{
	return PlaySample(0, 0, p_id, NULL, p_volume, g_soundConfig.m_effectsVolume, p_pan, p_rate, (MechS32*) -1, 0x450);
}

// FUNCTION: MW2 0x1007ea4c
MechS32 PlaySoundRandomRate(
	MechS32 p_delay,
	MechS32 p_bearing,
	MechS32 p_id,
	MechU32 p_volume,
	MechS32 p_pan,
	MechU16 p_flags
)
{
	return PlaySample(
		p_delay,
		p_bearing,
		p_id,
		NULL,
		p_volume,
		g_soundConfig.m_effectsVolume,
		p_pan,
		RandomSampleRate(),
		(MechS32*) -1,
		p_flags
	);
}

// Plays p_sound at the offset (p_dx, p_dy, p_dz) from the eyepoint, delayed by its distance
// and fading with it; p_half halves the volume. Returns the distance.
// Stack slots: horizontal, delay, pitch, volume, bearing, distance and range are permuted.
// FUNCTION: MW2 0x1007ea8c
MechS32 PlayPositionalSound(MechS32 p_dx, MechS32 p_dy, MechS32 p_dz, MechS32 p_sound, MechS32 p_half)
{
	MechS32 horizontal;
	MechS32 delay;
	MechS32 pitch;
	MechS32 volume;
	MechS32 bearing;
	MechS32 distance;
	MechS32 range;

	GetBearingAndRange(p_dx, p_dy, p_dz, &bearing, &distance, (MechU32*) &horizontal, &pitch);
	range = distance;
	volume = GetDistanceVolume(range);
	if (volume > 0) {
		if (p_half) {
			volume >>= 1;
		}

		delay = FixedMul16(distance, g_soundDelayPerUnit);
		PlayDelayedSound(delay, bearing, p_sound, volume, -1, 0x32);
	}

	return distance;
}

// FUNCTION: MW2 0x1007eb23
MechS32 PlaySoundEffect(MechS32 p_id, MechU32 p_volume, MechS32 p_pan, MechS32 p_rate, MechU16 p_flags)
{
	return PlaySample(
		0,
		0,
		p_id,
		NULL,
		p_volume,
		g_soundConfig.m_effectsVolume,
		p_pan,
		g_sampleRates[p_rate],
		(MechS32*) -1,
		p_flags
	);
}

// FUNCTION: MW2 0x1007eb64
MechS32 PlayDelayedSound(
	MechS32 p_delay,
	MechS32 p_bearing,
	MechS32 p_id,
	MechU32 p_volume,
	MechS32 p_pan,
	MechU16 p_flags
)
{
	return PlaySoundRandomRate(p_delay, p_bearing, p_id, p_volume, p_pan, p_flags);
}

// FUNCTION: MW2 0x1007ebd1
MechS32 PlaySoundAt(MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_sound, MechS32 p_half)
{
	return PlayPositionalSound(p_x, p_y, p_z, p_sound, p_half);
}

// FUNCTION: MW2 0x1007ebfd
MechS32 CalculateSamplePan(MechS32 p_bearing)
{
	MechS32 pan;
	MechS32 angle;

	angle = g_eyepoint->m_heading - p_bearing;
	pan = FixedSin(angle) >> 23;
	pan += 0x40;
	if (pan < 0xf) {
		pan = 0xf;
	}
	else if (pan > 0x6f) {
		pan = 0x6f;
	}

	return pan;
}

// FUNCTION: MW2 0x1007ec5e
MechS32 RandomSampleRate(void)
{
	return g_sampleRates[RandomIntBelow(10)];
}

// Stops the playing samples; p_all stops the speech sample too.
// FUNCTION: MW2 0x1007ec7f
void StopSamples(MechS32 p_all)
{
	MechS32 i;

	if (!g_audioEngine) {
		return;
	}

	for (i = 0; i < 16; i++) {
		if (g_audioEngine->m_playing[i]) {
			if (g_audioEngine->m_samples[i] == g_speechSample && !p_all) {
				continue;
			}

			AIL_end_sample(g_audioEngine->m_samples[i]);
			g_audioEngine->m_playing[i] = 0;
		}
	}
}

// Stack slots: i, x, started, distance and y are permuted.
// FUNCTION: MW2 0x1007ed1e
void UpdateAmbientSound(AmbientSound* p_sound)
{
	HSAMPLE sample;
	MechS32 i;
	MechS32 volume;
	MechS32 x;
	MechS32 started;
	MechS32 distance;
	MechS32 bearing;
	MechS32 y;
	MechS32 z;

	started = 0;
	if (!g_audioEngine) {
		return;
	}

	if (p_sound->m_skip) {
		p_sound->m_skip = 0;
		return;
	}

	GetObjPosition(p_sound->m_obj, &x, &y, &z);
	x = g_eyepoint->m_x - x;
	y = g_eyepoint->m_y - y;
	z = g_eyepoint->m_z - z;
	GetBearingAndRange(x, y, z, &bearing, &distance, (MechU32*) &i, &i);

	if (p_sound->m_range < distance) {
		StopAmbientSound(p_sound);
		return;
	}

	if (p_sound->m_slot == -1) {
		for (i = 8; i < 16; i++) {
			if (!g_audioEngine->m_playing[i]) {
				p_sound->m_slot = i;
				break;
			}
		}

		if (p_sound->m_slot == -1) {
			return;
		}

		if (!p_sound->m_data) {
			p_sound->m_data = LoadCachedResource(0, p_sound->m_id, g_resourceTypeTags[c_resTagSnds], 0);
			if (p_sound->m_data) {
			}
		}

		if (!p_sound->m_data ||
			(sample = AIL_allocate_file_sample(g_audioEngine->m_driver, p_sound->m_data, -1)) == 0) {
			if (p_sound->m_id != -1) {
				UnlockCachedResource(p_sound->m_id, g_resourceTypeTags[c_resTagSnds]);
			}

			p_sound->m_slot = -1;
			p_sound->m_data = NULL;
			return;
		}

		g_audioEngine->m_samples[p_sound->m_slot] = sample;
		g_audioEngine->m_ids[p_sound->m_slot] = p_sound->m_id;
		g_audioEngine->m_flags[p_sound->m_slot] = 0x1000;
		AIL_set_sample_type(sample, 0, 0);
		AIL_set_sample_loop_count(sample, 0);
		started = 1;
	}

	sample = g_audioEngine->m_samples[p_sound->m_slot];
	volume = GetDistanceVolume(distance);
	if (volume <= 0) {
		StopAmbientSound(p_sound);
		return;
	}

	volume = FixedMul16(volume, g_soundConfig.m_effectsVolume);
	volume >>= 2;
	AIL_set_sample_pan(sample, CalculateSamplePan(bearing));
	AIL_set_sample_volume(sample, volume);
	AIL_set_sample_playback_rate(sample, RandomSampleRate());

	if (started || !g_audioEngine->m_playing[p_sound->m_slot]) {
		AIL_start_sample(sample);
		g_audioEngine->m_playing[p_sound->m_slot] = 1;
	}
}

// FUNCTION: MW2 0x1007f01f
void StopAmbientSound(AmbientSound* p_sound)
{
	HSAMPLE sample;

	if (!p_sound || !p_sound->m_data) {
		return;
	}

	sample = g_audioEngine->m_samples[p_sound->m_slot];
	g_audioEngine->m_samples[p_sound->m_slot] = 0;
	g_audioEngine->m_playing[p_sound->m_slot] = 0;
	g_audioEngine->m_ids[p_sound->m_slot] = 0;
	p_sound->m_slot = -1;
	AIL_end_sample(sample);
	AIL_release_sample_handle(sample);
	UnlockCachedResource(p_sound->m_id, g_resourceTypeTags[c_resTagSnds]);
	p_sound->m_data = NULL;
}

// The volume, 0-100, at p_distance: full at 0, silent from 50000.
// FUNCTION: MW2 0x1007f0d9
MechS32 GetDistanceVolume(MechS32 p_distance)
{
	MechS32 volume;

	if (p_distance > 50000) {
		return 0;
	}

	p_distance /= 100;
	p_distance *= p_distance;
	volume = 100 - p_distance * 100 / 250000;
	return volume;
}
