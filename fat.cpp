#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include <list>

#include "fat.h"
#include "fat_file.h"

#include <unistd.h>


/**
 * Write inside one block in the filesystem.
 * @param  fs           filesystem
 * @param  block_id     index of block in the filesystem
 * @param  block_offset offset inside the block
 * @param  size         size to write, must be less than BLOCK_SIZE
 * @param  buffer       data buffer
 * @return              written byte count
 */
int mini_fat_write_in_block(FAT_FILESYSTEM *fs, const int block_id, const int block_offset, const int size, const void * buffer) {
	//printf("offset: %d, size: %d, file: %d, buff: %s, buffer size: %d\n", block_offset, size, fs->block_size, buffer, sizeof(buffer));
	assert(block_offset >= 0);
	assert(block_offset < fs->block_size);
	assert(size + block_offset <= fs->block_size);

	int written = 0;

	// TODO: write in the real file.
	FILE *fd = fopen(fs->filename, "r+b");
	fseek(fd, (block_id * fs->block_size) + block_offset, SEEK_SET);
	if(fwrite(buffer, size, 1, fd) > 0){
		written = size;
	}
	fclose(fd);
	return written;
}

/**
 * Read inside one block in the filesystem
 * @param  fs           filesystem
 * @param  block_id     index of block in the filesystem
 * @param  block_offset offset inside the block
 * @param  size         size to read, must fit inside the block
 * @param  buffer       buffer to write the read stuff to
 * @return              read byte count
 */
int mini_fat_read_in_block(FAT_FILESYSTEM *fs, const int block_id, const int block_offset, const int size, void * buffer) {
	assert(block_offset >= 0);
	assert(block_offset < fs->block_size);
	assert(size + block_offset <= fs->block_size);

	int read = 0;

	// TODO: read from the real file.
	FILE *fd = fopen(fs->filename, "r+");
	fseek(fd, (block_id * fs->block_size) + block_offset, SEEK_SET);
	if(fread(buffer, size, 1, fd) > 0){
		read = size;
	}
	fclose(fd);
	return read;
}


/**
 * Find the first empty block in filesystem.
 * @return -1 on failure, index of block on success
 */
int mini_fat_find_empty_block(const FAT_FILESYSTEM *fat) {
	// TODO: find an empty block in fat and return its index.
	for(int i = 0; i < fat->block_map.size(); i++){
		if(fat->block_map[i] == EMPTY_BLOCK){
			return i;
		} 
	}
	return -1;
}

/**
 * Find the first empty block in filesystem, and allocate it to a type,
 * i.e., set block_map[new_block_index] to the specified type.
 * @return -1 on failure, new_block_index on success
 */
int mini_fat_allocate_new_block(FAT_FILESYSTEM *fs, const unsigned char block_type) {
	int new_block_index = mini_fat_find_empty_block(fs);
	if (new_block_index == -1)
	{
		fprintf(stderr, "Cannot allocate block: filesystem is full.\n");
		return -1;
	}
	fs->block_map[new_block_index] = block_type;
	return new_block_index;
}

void mini_fat_dump(const FAT_FILESYSTEM *fat) {
	printf("Dumping fat with %d blocks of size %d:\n", fat->block_count, fat->block_size);
	for (int i=0; i<fat->block_count;++i) {
		printf("%d ", (int)fat->block_map[i]);
	}
	printf("\n");

	for (int i=0; i<fat->files.size(); ++i) {
		mini_file_dump(fat, fat->files[i]);
	}
}

static FAT_FILESYSTEM * mini_fat_create_internal(const char * filename, const int block_size, const int block_count) {
	FAT_FILESYSTEM * fat = new FAT_FILESYSTEM;
	fat->filename = filename;
	fat->block_size = block_size;
	fat->block_count = block_count;
	fat->block_map.resize(fat->block_count, EMPTY_BLOCK); // Set all blocks to empty.
	fat->block_map[0] = METADATA_BLOCK;
	return fat;
}

/**
 * Create a new virtual disk file.
 * The file should be of the exact size block_size * block_count bytes.
 * Overwrites existing files. Resizes block_map to block_count size.
 * @param  filename    name of the file on real disk
 * @param  block_size  size of each block
 * @param  block_count number of blocks
 * @return             FAT_FILESYSTEM pointer with parameters set.
 */
FAT_FILESYSTEM * mini_fat_create(const char * filename, const int block_size, const int block_count) {

	FAT_FILESYSTEM * fat = mini_fat_create_internal(filename, block_size, block_count);

	// TODO: create the corresponding virtual disk file with appropriate size.
    FILE *fd = fopen(filename, "wb");
    ftruncate(fileno(fd), block_size*block_count);
    fclose(fd);
	return fat;
}

/**
 * Save a virtual disk (filesystem) to file on real disk.
 * Stores filesystem metadata (e.g., block_size, block_count, block_map, etc.)
 * in block 0.
 * Stores file metadata (name, size, block map) in their corresponding blocks.
 * Does not store file data (they are written directly via write API).
 * @param  fat virtual disk filesystem
 * @return     true on success
 */
bool mini_fat_save(const FAT_FILESYSTEM *fat) {
	FILE * fat_fd = fopen(fat->filename, "rb+");
	if (fat_fd == NULL) {
		perror("Cannot save fat to file");
		return false;
	}
	// TODO: save all metadata (filesystem metadata, file metadata).
	
	fwrite(&(fat->block_count), sizeof(int), 1, fat_fd);
	fwrite(&(fat->block_size), sizeof(int), 1, fat_fd);
	fwrite(&(fat->block_map), sizeof(fat->block_map), 1, fat_fd);
    int file_index = 0;
    for(int i = 0; i < fat->block_map.size(); i++){
        if(fat->block_map[i] == FILE_ENTRY_BLOCK){
            FAT_FILE* fd = fat->files[file_index];
            int size = fd->size;
            int name_lenght = strlen(fd->name);
            fseek(fat_fd, (fd->metadata_block_id * fat->block_size), SEEK_SET);
            fwrite(&size, sizeof(int), 1, fat_fd);
            fwrite(&name_lenght, sizeof(int), 1, fat_fd);
            fwrite(&(fd->name), name_lenght, 1, fat_fd);
            fwrite(&(fd->block_ids), sizeof(fd->block_ids), 1, fat_fd);
            file_index++;
        }
    }
	fclose(fat_fd);
	return true;
}

FAT_FILESYSTEM * mini_fat_load(const char *filename) {
	FILE * fat_fd = fopen(filename, "rb+");
	if (fat_fd == NULL) {
		perror("Cannot load fat from file");
		exit(-1);
	}
	// TODO: load all metadata (filesystem metadata, file metadata) and create filesystem.

    int blocksize;
    int blockcount;
	fread(&(blockcount), sizeof(int), 1, fat_fd);
	fread(&(blocksize), sizeof(int), 1, fat_fd);
    FAT_FILESYSTEM * fat = mini_fat_create_internal(filename, blocksize, blockcount);
	fread(&(fat->block_map), sizeof(fat->block_map), 1, fat_fd);
    for(int i = 0; i < fat->block_map.size(); i++){
        if(fat->block_map[i] == FILE_ENTRY_BLOCK){
            FAT_FILE* fd = new FAT_FILE;
            int name_size;
            fseek(fat_fd, (i * fat->block_size), SEEK_SET);
            fread(&(fd->size), sizeof(int), 1, fat_fd);
            fread(&name_size, sizeof(int), 1, fat_fd);
            fread(&(fd->name), name_size, 1, fat_fd);
            fread(&(fd->block_ids), sizeof(fd->block_ids), 1, fat_fd);
            fat->files.push_back(fd);
        }
    }
	fclose(fat_fd);
	return fat;
}
