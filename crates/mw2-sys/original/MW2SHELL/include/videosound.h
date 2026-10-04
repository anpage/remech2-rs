#ifndef VIDEOSOUND_H
#define VIDEOSOUND_H

#include "decomp.h"
#include "mss.h"
#include "types.h"

class AudioSubsystem;

// Per-video sound object held by the FMV slot table (FmvSlot::m_sound).
// The matched streaming methods establish a 0x14-byte floor, not the full size.
class VideoSound {
public:
	VideoSound(AudioSubsystem* p_subsystem, MechS32 p_stereo, MechS32 p_wide, MechS32 p_size);
	~VideoSound();
	undefined4 IsBufferReady();
	void* GetReadyBuffer();
	void LoadBuffer(void* p_buffer, MechU32 p_size);

private:
	AudioSubsystem* m_subsystem; // 0x00
	HSAMPLE m_sample;            // 0x04 — Miles sample handle
	void* m_buffer0;             // 0x08 — heap buffer
	void* m_buffer1;             // 0x0c — heap buffer
	MechS32 m_readyBuffer;       // 0x10 — ready buffer index, -1 until queried
};

#endif // VIDEOSOUND_H
