#ifndef FILES_H
#define FILES_H

// The game's access to its files, implemented on the Rust side (src/files.rs). A path is relative
// to the game's directory unless it's absolute, may use either slash, and matches the names on
// disk whatever their case: a file about to be created takes the place of one whose name differs
// only by case.

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

	// fopen
	FILE* MechFopen(const char* p_path, const char* p_mode);
	// How MechOpen opens a file: always as binary
	enum {
		c_mechOpenRead,      // to read
		c_mechOpenReadWrite, // to read and write
		c_mechOpenCreate     // to write, creating it if it isn't there, without truncating it
	};

	// open, for the C library's read, write and close. Returns -1 on failure.
	int MechOpen(const char* p_path, int p_mode);
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

#ifdef __cplusplus
}
#endif

#endif // FILES_H
