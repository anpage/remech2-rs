/* Smacker (SMACKW32.DLL) declarations: only what the game calls, with the types its matched
   callers prove. There is no SDK in the tree; add functions here as callers are decompiled.
   The import library is generated from smackw32.def; the original imports the functions by
   ordinal through the __cdecl thunks at 0x100492a2-0x100492de. */
#ifndef SMACKW32_H
#define SMACKW32_H

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct SMACK_TAG {
		unsigned int Version;                  /* 0x00 */
		unsigned int Width;                    /* 0x04 */
		unsigned int Height;                   /* 0x08 */
		int Frames;                            /* 0x0c: signed in the game's frame-loop comparison */
		unsigned char Unknown10[0x68 - 0x10];  /* 0x10 */
		unsigned int NewPalette;               /* 0x68: set when the frame changed the palette */
		unsigned int PalType;                  /* 0x6c: 1 when Palette holds the palette */
		unsigned char Palette[0x374 - 0x70];   /* 0x70 */
		unsigned char AlternatePalette[0x300]; /* 0x374: used when PalType isn't 1 */
	} Smack;

	Smack* SmackOpen(const char* p_name, unsigned int p_flags, unsigned int p_extraBuffers);
	void SmackGoto(Smack* p_smack, int p_frame);
	void SmackDoFrame(Smack* p_smack);
	void SmackNextFrame(Smack* p_smack);
	void SmackClose(Smack* p_smack);
	unsigned short SmackWait(Smack* p_smack);
	unsigned short SmackSoundInTrack(Smack* p_smack, unsigned int p_track);
	void SmackToBuffer(
		Smack* p_smack,
		unsigned int p_left,
		unsigned int p_top,
		unsigned int p_pitch,
		unsigned int p_height,
		void* p_buffer,
		unsigned int p_flags
	);
	unsigned int SmackGetTrackData(Smack* p_smack, void* p_buffer, unsigned int p_track);

#ifdef __cplusplus
}
#endif

#endif /* SMACKW32_H */
