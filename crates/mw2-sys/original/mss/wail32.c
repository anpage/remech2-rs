/* Symbol source for the WAIL32 import library (see wail32.def). Only the __stdcall decoration,
   i.e. the argument byte count, matters: LIB reads this object to resolve the .def names, and it
   is never linked into a game DLL. The real prototypes belong in our Miles header. */

#define IMPORT0(name)                                                                                                  \
	void __stdcall name(void)                                                                                          \
	{                                                                                                                  \
	}
#define IMPORT1(name)                                                                                                  \
	void __stdcall name(int p_0)                                                                                       \
	{                                                                                                                  \
	}
#define IMPORT2(name)                                                                                                  \
	void __stdcall name(int p_0, int p_1)                                                                              \
	{                                                                                                                  \
	}
#define IMPORT3(name)                                                                                                  \
	void __stdcall name(int p_0, int p_1, int p_2)                                                                     \
	{                                                                                                                  \
	}
#define IMPORT4(name)                                                                                                  \
	void __stdcall name(int p_0, int p_1, int p_2, int p_3)                                                            \
	{                                                                                                                  \
	}
#define IMPORT5(name)                                                                                                  \
	void __stdcall name(int p_0, int p_1, int p_2, int p_3, int p_4)                                                   \
	{                                                                                                                  \
	}

IMPORT1(AIL_active_sample_count)
IMPORT1(AIL_active_sequence_count)
IMPORT3(AIL_allocate_file_sample)
IMPORT1(AIL_allocate_sample_handle)
IMPORT1(AIL_allocate_sequence_handle)
IMPORT1(AIL_end_sample)
IMPORT1(AIL_init_sample)
IMPORT3(AIL_init_sequence)
IMPORT4(AIL_load_sample_buffer)
IMPORT1(AIL_lock_channel)
IMPORT3(AIL_midiOutOpen)
IMPORT3(AIL_minimum_sample_buffer_size)
IMPORT2(AIL_register_EOS_callback)
IMPORT1(AIL_register_timer)
IMPORT2(AIL_release_channel)
IMPORT1(AIL_release_sample_handle)
IMPORT1(AIL_release_sequence_handle)
IMPORT1(AIL_release_timer_handle)
IMPORT1(AIL_resume_sample)
IMPORT1(AIL_resume_sequence)
IMPORT1(AIL_sample_buffer_ready)
IMPORT1(AIL_sample_status)
IMPORT2(AIL_sample_user_data)
IMPORT5(AIL_send_channel_voice_message)
IMPORT1(AIL_sequence_status)
IMPORT0(AIL_serve)
IMPORT2(AIL_set_preference)
IMPORT3(AIL_set_sample_file)
IMPORT2(AIL_set_sample_loop_count)
IMPORT2(AIL_set_sample_pan)
IMPORT2(AIL_set_sample_playback_rate)
IMPORT3(AIL_set_sample_type)
IMPORT3(AIL_set_sample_user_data)
IMPORT2(AIL_set_sample_volume)
IMPORT2(AIL_set_sequence_loop_count)
IMPORT3(AIL_set_sequence_volume)
IMPORT2(AIL_set_timer_divisor)
IMPORT2(AIL_set_timer_period)
IMPORT0(AIL_shutdown)
IMPORT1(AIL_start_sample)
IMPORT1(AIL_start_sequence)
IMPORT1(AIL_start_timer)
IMPORT0(AIL_startup)
IMPORT1(AIL_stop_sample)
IMPORT1(AIL_stop_sequence)
IMPORT1(AIL_waveOutClose)
IMPORT4(AIL_waveOutOpen)
IMPORT2(FILE_read)
IMPORT1(MEM_free_lock)
