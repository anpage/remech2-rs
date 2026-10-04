/* Miles Sound System (WAIL32.DLL) declarations: only what the game calls, with the types its
   matched callers prove. There is no SDK in the tree; add functions here as callers are
   decompiled. The functions are implemented on the Rust side, so nothing here depends on
   Windows. */
#ifndef MSS_H
#define MSS_H

// The sample user data: 32-bit in this Miles, pointer-sized (SINTa) from later versions on. The game
// keeps pointers there, so it is pointer-sized where pointers are wider than 32 bits.
#if defined(_MSC_VER) && _MSC_VER < 1200
#define SINTa int
#else
#include <stdint.h>
#define SINTa intptr_t
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// __stdcall like the original where that convention exists (Rust's extern "system")
#if defined(_M_IX86) || defined(__i386__)
#define AILCALL __stdcall
#else
#define AILCALL
#endif
#define AILIMPORT

	// Stand-ins for the Win32 multimedia types Miles took: the wave and MIDI device handles it
	// hands back, and the PCM format it opens the wave device with.
	typedef void* AILWAVEOUT;
	typedef void* AILMIDIOUT;

#define AIL_WAVE_FORMAT_PCM 1

#pragma pack(push, 1)
	typedef struct AILWAVEFORMAT {
		unsigned short wFormatTag;
		unsigned short nChannels;
		unsigned int nSamplesPerSec;
		unsigned int nAvgBytesPerSec;
		unsigned short nBlockAlign;
	} AILWAVEFORMAT;

	typedef struct AILPCMWAVEFORMAT {
		AILWAVEFORMAT wf;
		unsigned short wBitsPerSample;
	} AILPCMWAVEFORMAT;
#pragma pack(pop)

	typedef struct _MDI_DRIVER* HMDIDRIVER;
	typedef struct _SEQUENCE* HSEQUENCE;
	typedef struct _SAMPLE* HSAMPLE;
	typedef struct _DIG_DRIVER* HDIGDRIVER;

#define SMP_PLAYING 4

#define SEQ_DONE 2
#define SEQ_PLAYING 4
#define SEQ_STOPPED 8

#define AILCALLBACK AILCALL
	typedef void(AILCALLBACK* AILSAMPLECB)(HSAMPLE p_sample);

	AILIMPORT void AILCALL AIL_shutdown(void);

	AILIMPORT int AILCALL AIL_midiOutOpen(HMDIDRIVER* p_driver, AILMIDIOUT** p_midiOut, int p_deviceId);
	AILIMPORT int AILCALL AIL_set_preference(unsigned int p_number, int p_value);
	AILIMPORT int AILCALL AIL_lock_channel(HMDIDRIVER p_driver);
	AILIMPORT void AILCALL AIL_release_channel(HMDIDRIVER p_driver, int p_channel);
	AILIMPORT void AILCALL
	AIL_send_channel_voice_message(HMDIDRIVER p_driver, HSEQUENCE p_sequence, int p_status, int p_data1, int p_data2);

	AILIMPORT int AILCALL
	AIL_waveOutOpen(HDIGDRIVER* p_driver, AILWAVEOUT** p_waveOut, int p_deviceId, AILWAVEFORMAT* p_format);

	AILIMPORT HSEQUENCE AILCALL AIL_allocate_sequence_handle(HMDIDRIVER p_driver);
	AILIMPORT void AILCALL AIL_release_sequence_handle(HSEQUENCE p_sequence);
	AILIMPORT int AILCALL AIL_init_sequence(HSEQUENCE p_sequence, void* p_start, int p_sequenceNum);
	AILIMPORT void AILCALL AIL_start_sequence(HSEQUENCE p_sequence);
	AILIMPORT void AILCALL AIL_stop_sequence(HSEQUENCE p_sequence);
	AILIMPORT void AILCALL AIL_resume_sequence(HSEQUENCE p_sequence);
	AILIMPORT int AILCALL AIL_sequence_status(HSEQUENCE p_sequence);
	AILIMPORT void AILCALL AIL_set_sequence_volume(HSEQUENCE p_sequence, int p_volume, int p_milliseconds);

	AILIMPORT HSAMPLE AILCALL AIL_allocate_sample_handle(HDIGDRIVER p_driver);
	AILIMPORT void AILCALL AIL_release_sample_handle(HSAMPLE p_sample);
	AILIMPORT void AILCALL AIL_init_sample(HSAMPLE p_sample);
	AILIMPORT void AILCALL AIL_start_sample(HSAMPLE p_sample);
	AILIMPORT void AILCALL AIL_stop_sample(HSAMPLE p_sample);
	AILIMPORT void AILCALL AIL_resume_sample(HSAMPLE p_sample);
	AILIMPORT void AILCALL AIL_end_sample(HSAMPLE p_sample);
	AILIMPORT void AILCALL AIL_set_sample_volume(HSAMPLE p_sample, int p_volume);
	AILIMPORT void AILCALL AIL_set_sample_loop_count(HSAMPLE p_sample, int p_loopCount);
	AILIMPORT void AILCALL AIL_set_sample_pan(HSAMPLE p_sample, int p_pan);
	AILIMPORT void AILCALL AIL_set_sample_playback_rate(HSAMPLE p_sample, int p_rate);
	AILIMPORT void AILCALL AIL_set_sample_user_data(HSAMPLE p_sample, unsigned int p_index, SINTa p_value);
	AILIMPORT SINTa AILCALL AIL_sample_user_data(HSAMPLE p_sample, unsigned int p_index);
	AILIMPORT AILSAMPLECB AILCALL AIL_register_EOS_callback(HSAMPLE p_sample, AILSAMPLECB p_callback);
	AILIMPORT HSAMPLE AILCALL AIL_allocate_file_sample(HDIGDRIVER p_driver, void* p_fileImage, int p_block);

	AILIMPORT void AILCALL AIL_serve(void);

	AILIMPORT void AILCALL AIL_set_sample_type(HSAMPLE p_sample, int p_format, unsigned int p_flags);
	AILIMPORT int AILCALL AIL_sample_buffer_ready(HSAMPLE p_sample);
	AILIMPORT void AILCALL
	AIL_load_sample_buffer(HSAMPLE p_sample, unsigned int p_bufferNum, void* p_buffer, unsigned int p_size);

#ifdef __cplusplus
}
#endif

#endif /* MSS_H */
