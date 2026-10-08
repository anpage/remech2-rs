#ifndef FILES_H
#define FILES_H

// The game's access to its files, implemented on the Rust side (src/files.rs). A path is relative
// to the game's directory unless it's absolute, may use either slash, and matches the names on
// disk whatever their case: a file about to be created takes the place of one whose name differs
// only by case.

#include "heap.h"
#include "types.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

	// How MechOpen opens a file: always as binary
	enum {
		c_mechOpenRead,      // to read
		c_mechOpenReadWrite, // to read and write
		c_mechOpenCreate,    // to write, creating it if it isn't there, without truncating it
		c_mechOpenWrite      // to write, creating or emptying it
	};

	// open. Returns a handle for the functions below, or -1 on failure.
	int MechOpen(const char* p_path, int p_mode);
	// read and write. Return the number of bytes read or written, or -1 on failure.
	int MechRead(int p_file, void* p_buffer, unsigned int p_count);
	int MechWrite(int p_file, const void* p_buffer, unsigned int p_count);
	// lseek, with stdio's SEEK_SET, SEEK_CUR or SEEK_END. Returns the new position, or -1.
	MechS32 MechSeek(int p_file, MechS32 p_offset, int p_origin);
	// close. Returns 0, or -1 for a handle that isn't open.
	int MechClose(int p_file);
	// The file's size, or -1
	MechS32 MechFileLength(int p_file);
	// remove, rename and mkdir. Return 0, or -1 on failure.
	int MechRemove(const char* p_path);
	int MechRename(const char* p_from, const char* p_to);
	int MechMakeDir(const char* p_path);
	// Non-zero for a file or directory that exists
	int MechFileExists(const char* p_path);

	// The files matching a pattern, whose last component may have the wildcards * and ?
	// ("keating\\*.sfl"): their names without the directory, as they are on disk, in sorted order.
	typedef struct MechFileList MechFileList;

	// Never NULL: an empty list when nothing matches
	MechFileList* MechFindFiles(const char* p_pattern);
	size_t MechFileListCount(const MechFileList* p_list);
	// NULL past the end
	const char* MechFileListName(const MechFileList* p_list, size_t p_index);
	void MechFileListFree(MechFileList* p_list);

	void* MechReadFile(MechHeap* p_heap, const char* p_path);

#ifdef __cplusplus
}
#endif

#endif // FILES_H
