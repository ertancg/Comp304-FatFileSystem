#ifndef FAT_H
#define FAT_H

#include <vector>

typedef struct t_FAT_FILE FAT_FILE; // Forward definition.

const unsigned char EMPTY_BLOCK = 0;
const unsigned char FILE_ENTRY_BLOCK = 1;
const unsigned char FILE_DATA_BLOCK = 2;
const unsigned char METADATA_BLOCK = 3; // Only for the first block.

// Feel free to modify this structure.
typedef struct t_FAT_FILESYSTEM {
	const char * filename;
	int block_count;
	int block_size;
	std::vector<unsigned char> block_map;

	std::vector<FAT_FILE*> files;
} FAT_FILESYSTEM;


/// Public APIs
// DO NOT MODIFY THE FOLLOWING:
FAT_FILESYSTEM * mini_fat_create(const char * filename, const int block_size, const int block_count);
bool mini_fat_save(const FAT_FILESYSTEM *fat);
FAT_FILESYSTEM * mini_fat_load(const char *filename);
void mini_fat_dump(const FAT_FILESYSTEM *fat);


// Helpers (not mandatory):
int mini_fat_find_empty_block(const FAT_FILESYSTEM *fat);
int mini_fat_allocate_new_block(FAT_FILESYSTEM *fs, const unsigned char block_type);
int mini_fat_write_in_block(FAT_FILESYSTEM *fs, const int block_id, const int block_offset, const int size, const void * buffer);
int mini_fat_read_in_block(FAT_FILESYSTEM *fs, const int block_id, const int block_offset, const int size, void * buffer);


#endif //FAT_H