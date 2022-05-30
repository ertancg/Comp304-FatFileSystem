#ifndef FAT_FILE_H
#define FAT_FILE_H


#include <vector>

const int MAX_FILENAME_LENGTH = 256;

// Feel free to modify the following structure.
typedef struct t_FAT_OPEN_FILE {
	FAT_FILE * file; // Pointers to FAT_FILE structure (the actual file).
	int position; // Seek position.
	bool is_write;
} FAT_OPEN_FILE;

// Feel free to modify the following structure.
typedef struct t_FAT_FILE {
	char name[MAX_FILENAME_LENGTH];
	int size;
	int metadata_block_id; // The block index that holds the metadata of this file (entry block).
	std::vector<int> block_ids; // Data blocks.

	std::vector<const FAT_OPEN_FILE*> open_handles; // One entry each time this file is opened.
} FAT_FILE;

typedef struct t_FAT_FILESYSTEM FAT_FILESYSTEM; // Forward definition.


/// Public APIs
// DO NOT MODIFY THE FOLLOWING:
void mini_file_dump(const FAT_FILESYSTEM *fs, const FAT_FILE *file);
FAT_OPEN_FILE * mini_file_open(FAT_FILESYSTEM *fs, const char *filename, const bool is_write);
bool mini_file_close(FAT_FILESYSTEM *fs, const FAT_OPEN_FILE * open_file);

bool mini_file_seek(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int offset, const bool from_start);
bool mini_file_delete(FAT_FILESYSTEM *fs, const char *filename);
int mini_file_size(FAT_FILESYSTEM *fs, const char *filename);

int mini_file_read(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int size, void * buffer);
int mini_file_write(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int size, const void * buffer);


// Helpers (not mandatory):
FAT_FILE * mini_file_create_file(FAT_FILESYSTEM *fs, const char *filename);
FAT_FILE * mini_file_create(const char * filename);
FAT_FILE * mini_file_find(const FAT_FILESYSTEM *fs, const char *filename);

inline int position_to_block_index(const FAT_FILESYSTEM * fs, const int position)  {
	return position / fs->block_size;
}
inline int position_to_byte_index(const FAT_FILESYSTEM * fs, const int position) {
	return position % fs->block_size;
}

#endif // FAT_FILE_H