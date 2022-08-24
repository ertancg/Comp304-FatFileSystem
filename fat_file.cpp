#include "fat.h"
#include "fat_file.h"
#include <string.h>
#include <stdlib.h>

// Little helper to show debug messages. Set 1 to 0 to silence.
#define DEBUG 1
inline void debug(const char * fmt, ...) {
#if DEBUG>0
	va_list args;
   va_start(args, fmt);
   vprintf(fmt, args);
   va_end(args);
#endif
}

// Delete index-th item from vector.
template<typename T>
static void vector_delete_index(std::vector<T> &vector, const int index) {
	vector.erase(vector.begin() + index);
}

// Find var and delete from vector.
template<typename T>
static bool vector_delete_value(std::vector<T> &vector, const T var) {
	for (int i=0; i<vector.size(); ++i) {
		if (vector[i] == var) {
			vector_delete_index(vector, i);
			return true;
		}
	}
	return false;
}

void mini_file_dump(const FAT_FILESYSTEM *fs, const FAT_FILE *file)
{
	printf("Filename: %s\tFilesize: %d\tBlock count: %d\n", file->name, file->size, (int)file->block_ids.size());
	printf("\tMetadata block: %d\n", file->metadata_block_id);
	printf("\tBlock list: ");
	for (int i=0; i<file->block_ids.size(); ++i) {
		printf("%d ", file->block_ids[i]);
	}
	printf("\n");

	printf("\tOpen handles: \n");
	for (int i=0; i<file->open_handles.size(); ++i) {
		printf("\t\t%d) Position: %d (Block %d, Byte %d), Is Write: %d\n", i,
			file->open_handles[i]->position,
			position_to_block_index(fs, file->open_handles[i]->position),
			position_to_byte_index(fs, file->open_handles[i]->position),
			file->open_handles[i]->is_write);
	}
}


/**
 * Find a file in loaded filesystem, or return NULL.
 */
FAT_FILE * mini_file_find(const FAT_FILESYSTEM *fs, const char *filename)
{
	for (int i=0; i<fs->files.size(); ++i) {
		if (strcmp(fs->files[i]->name, filename) == 0) // Match
			return fs->files[i];
	}
	return NULL;
}

/**
 * Create a FAT_FILE struct and set its name.
 */
FAT_FILE * mini_file_create(const char * filename)
{
	FAT_FILE * file = new FAT_FILE;
	file->size = 0;
	strcpy(file->name, filename);
	return file;
}


/**
 * Create a file and attach it to filesystem.
 * @return FAT_OPEN_FILE pointer on success, NULL on failure
 */
FAT_FILE * mini_file_create_file(FAT_FILESYSTEM *fs, const char *filename)
{
	assert(strlen(filename)< MAX_FILENAME_LENGTH);
	FAT_FILE *fd = mini_file_create(filename);

	int new_block_index = mini_fat_allocate_new_block(fs, FILE_ENTRY_BLOCK);
	if (new_block_index == -1)
	{
		fprintf(stderr, "Cannot create new file '%s': filesystem is full.\n", filename);
		return NULL;
	}
	fs->files.push_back(fd); // Add to filesystem.
	fd->metadata_block_id = new_block_index;
	return fd;
}

/**
 * Return filesize of a file.
 * @param  fs       filesystem
 * @param  filename name of file
 * @return          file size in bytes, or zero if file does not exist.
 */
int mini_file_size(FAT_FILESYSTEM *fs, const char *filename) {
	FAT_FILE * fd = mini_file_find(fs, filename);
	if (!fd) {
		fprintf(stderr, "File '%s' does not exist.\n", filename);
		return 0;
	}
	return fd->size;
}


/**
 * Opens a file in filesystem.
 * If the file does not exist, returns NULL, unless it is write mode, where
 * the file is created.
 * Adds the opened file to file's open handles.
 * @param  is_write whether it is opened in write (append) mode or read.
 * @return FAT_OPEN_FILE pointer on success, NULL on failure
 */
FAT_OPEN_FILE * mini_file_open(FAT_FILESYSTEM *fs, const char *filename, const bool is_write)
{
	FAT_FILE * fd = mini_file_find(fs, filename);
	if (!fd) {
		// Check if it's write mode, and if so create it. Otherwise return NULL.
		if(is_write){
			fd = mini_file_create_file(fs, filename);
			if(!fd){
				return NULL;
			}
		}else{
			return NULL;
		}
	}

	if (is_write) {
		// Check if other write handles are open.
		for(int i = 0; i < fd->open_handles.size(); i++){
			if(fd->open_handles[i]->is_write){
				return NULL;
			}
		}
	}

	FAT_OPEN_FILE * open_file = new FAT_OPEN_FILE;
	// Assign open_file fields.
	open_file->file = fd;
	open_file->position = 0;
	open_file->is_write = is_write;
    
	// Add to list of open handles for fd:
	fd->open_handles.push_back(open_file);
	return open_file;
}

/**
 * Close an existing open file handle.
 * @return false on failure (no open file handle), true on success.
 */
bool mini_file_close(FAT_FILESYSTEM *fs, const FAT_OPEN_FILE * open_file)
{
	if (open_file == NULL) return false;
	FAT_FILE * fd = open_file->file;
	if (vector_delete_value(fd->open_handles, open_file)) {
		return true;
	}

	fprintf(stderr, "Attempting to close file that is not open.\n");
	return false;
}

/**
 * Write size bytes from buffer to open_file, at current position.
 * @return           number of bytes written.
 */
int mini_file_write(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int size, const void * buffer)
{
    int written_bytes = 0;
    // If file doesn't exist or it is not in write mode no write will happen.
    if(!open_file || !open_file->file){
        return written_bytes;
    }
    //Initial fiels to keep track of writing.
    int current_byte = 0;
    int current_block;
    int current_size;
    char* current_buffer = (char *) buffer;
    
    while(written_bytes < size){
        //If the position pointer in start of a new block, there will be new block allocation.
        if(position_to_byte_index(fs, open_file->position) == 0){
            //Allocate an empty data block.
            current_block = mini_fat_allocate_new_block(fs, FILE_DATA_BLOCK);
            if(current_block == -1){
                //There is no space available.
                return false;
            }else{
                //If created update the file struct.
                open_file->file->block_ids.push_back(current_block);
            }
            
            //If there is more data to be written than the block size (meaning a new block allocation is needed) current_size is changed accordingly
            if(size - written_bytes > fs->block_size){
                current_size = fs->block_size;
            }else{
                //Rest of the data can fit in the block.
                current_size = size - written_bytes;
            }
            
            current_byte = mini_fat_write_in_block(fs, current_block, 0, current_size, current_buffer);
            open_file->file->size += current_byte;
            written_bytes += current_byte;
            current_buffer = current_buffer + current_byte;
            mini_file_seek(fs, open_file, current_byte, false);
        }else{
            //If the position pointer is at the middle of a file we need to fill the remaining block first.
            current_block = open_file->file->block_ids[position_to_block_index(fs, open_file->position)];
            //If the data can't fit to the block current_size whould be empty space.
            if(size - written_bytes > fs->block_size - position_to_byte_index(fs, open_file->position)){
                current_size = fs->block_size - position_to_byte_index(fs, open_file->position);
            }else{
                //Data can fit in it.
                current_size = size - written_bytes;
            }
            current_byte = mini_fat_write_in_block(fs, current_block, position_to_byte_index(fs, open_file->position), current_size, current_buffer);
            written_bytes += current_byte;
            open_file->file->size += current_byte;
            current_buffer = current_buffer + current_byte;
            mini_file_seek(fs, open_file, current_byte, false);
        }
        
    }
   
    return written_bytes;
}

/**
 * Read up to size bytes from open_file into buffer.
 * @return           number of bytes read.
 */
int mini_file_read(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int size, void * buffer)
{
	int read_bytes = 0;
    //If file does not exist or is not in read mode no read will be perform.
	if(!open_file || !open_file->file || open_file->file->size == 0){
        return read_bytes;
    }
	// Initial fields to keep track of reading.
    int current_block;
    int current_size;
    int current_bytes;
    char* current_buffer = (char *) buffer;
    
    while(read_bytes < size){
        //If reading position is at the start of block.
        if(position_to_byte_index(fs, open_file->position) == 0){
            //Get the current block id that the pointer is in.
            current_block = open_file->file->block_ids[position_to_block_index(fs, open_file->position)];
            //If we are at the end of the file than rest of the file then the remaining space will be the current size.
            if(open_file->file->size - open_file->position < fs->block_size){
                current_size = open_file->file->size - open_file->position;
            }else{
                //If there is more data to write than the block size than current size is block size.
                if(size - read_bytes > fs->block_size){
                    current_size = fs->block_size;
                }else{
                    //If not current size what is need to be read.
                    current_size = size - read_bytes;
                }
            }
            
            current_bytes = mini_fat_read_in_block(fs, current_block, position_to_byte_index(fs, open_file->position), current_size, current_buffer);
            read_bytes += current_bytes;
            current_buffer = current_buffer + current_bytes;
            mini_file_seek(fs, open_file, current_bytes, false);
            
            //If the position pointer is at the end of the file returned size is adjusted with the full block size.
            if(open_file->file->size == open_file->position){
                return read_bytes + (fs->block_size - current_bytes);
            }
        }else{
            //Get the current block where the position pointer is resting.
            current_block = open_file->file->block_ids[position_to_block_index(fs, open_file->position)];
            if(size - read_bytes > fs->block_size - position_to_byte_index(fs, open_file->position)){
                current_size = fs->block_size - position_to_byte_index(fs, open_file->position);
            }else{
                current_size = size - read_bytes;
            }
            current_bytes = mini_fat_read_in_block(fs, current_block, position_to_byte_index(fs, open_file->position), current_size, current_buffer);
            read_bytes += current_bytes;
            current_buffer = current_buffer + current_bytes;
            mini_file_seek(fs, open_file, current_bytes, false);
        }
    }
    
    
    
	return read_bytes;
}


/**
 * Change the cursor position of an open file.
 * @param  offset     how much to change
 * @param  from_start whether to start from beginning of file (or current position)
 * @return            false if the new position is not available, true otherwise.
 */
bool mini_file_seek(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int offset, const bool from_start)
{
	// If file doesn not exist return false.
    if(!open_file || !open_file->file){
        return false;
    }
    //If offset causes overflow return false.
	if(open_file->position + offset > open_file->file->size){
		return false;
	}
    
    //Check whether the calculation would be done from the start or not.
	if(from_start){
        //If offset causes underflow return false.
        if(offset < 0){
            return false;
        }else{
            open_file->position = offset;
            return true;
        }
	}else{
        //If offset causes underflow return false.
        if(open_file->position + offset < 0){
            return false;
        }else{
            open_file->position += offset;
            return true;
        }
	}
}

/**
 * Attemps to delete a file from filesystem.
 * If the file is open, it cannot be deleted.
 * Marks the blocks of a deleted file as empty on the filesystem.
 * @return true on success, false on non-existing or open file.
 */
bool mini_file_delete(FAT_FILESYSTEM *fs, const char *filename)
{
	FAT_FILE *fd = mini_file_find(fs, filename);
	if(!fd){
		return false;
	}
    //If there is open_handles exist return false
	if(fd->open_handles.size() > 0){
		return false;
	}
    //Assign the metadata block with EMPTY_BLOCK
	fs->block_map[fd->metadata_block_id] = EMPTY_BLOCK;
	for(int i = 0; i < fd->block_ids.size(); i++){
        //Remove data part of block from the filesystem reference.
		fs->block_map[fd->block_ids[i]] = EMPTY_BLOCK;
	}
	vector_delete_value(fs->files, fd);

	return true;
}






