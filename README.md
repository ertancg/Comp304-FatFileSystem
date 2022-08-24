MINI FILE SYSTEM.

This is an file system API which operates on a virtual disk.
-mini_fat_save(fs): Writes the metadata of the system and files in to the 'physical' file on the system according the abstraction we've implemented.
-mini_fat_load(fs): Reads from the actual data file and constructs the needed data structes of system we've designed.
-mini_fat_write_in_block(FAT_FILESYSTEM *fs, const int block_id, const int block_offset, const int size, const void * buffer): Takes an offset and block number to write data on the system.
-mini_fat_read_in_block(FAT_FILESYSTEM *fs, const int block_id, const int block_offset, const int size, const void * buffer): Takes an offset and block number to read the data from the system.
-mini file seek(fs, open file, offset, from start): Changes the position pointer of the open file descriptor. Returns false if the change is not bound to constraints. 
-mini file delete(fs, filename): Removes the files reference from the system.
-mini file write(fs, open file, size, buffer): Continues to write from the position of the open file descriptor. Returns the total number of bytes written.
-mini file read(fs, open file, size, buffer): Continues to read from the position of the open file descriptor. Returns the total number of bytes read.
How to Run:
1. First compile the program with running command "make".
2. Then enter the command "./minifs" to run the mini file system.