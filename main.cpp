#include "fat.h"
#include "fat_file.h"

const char * fox = "The quick brown fox jumps over the lazy dog.\n";

int total_score = 0;
int current_score = 0;
inline void score2(const bool cond, const char * fmt, ...)
{
	va_list args;
   va_start(args, fmt);
   vprintf(fmt, args);
   va_end(args);
	current_score += cond;
	total_score++;
}
inline void score(const bool cond, int points = 1) {
	current_score += cond*points;
	total_score+=points;
	if (cond)
		printf("  => Pass\n");
	else
		printf("  => Fail\n");
}

void test_small_filesystem(FAT_FILESYSTEM * fs) {
	FAT_OPEN_FILE *fd1, *fd2, *fd3;
	printf("Openning 1st file in write mode should work.\n");
	fd1 = mini_file_open(fs, "file1.txt", true);
	score(fd1 != NULL);

	printf("Openning 2nd file in write mode should work.\n");
	fd2 = mini_file_open(fs, "file2.txt", true);
	score(fd2 != NULL);

	printf("Openning 3rd file in write mode should not work:\n");
	fd3 = mini_file_open(fs, "file3.txt", true);
	score(fd3 == NULL);
}

void test_open_3_files(FAT_FILESYSTEM * fs) {
	FAT_OPEN_FILE *fd1, *fd2, *fd3, *fd4, *fd5, *fd6;
	// Openning three files:
	printf("Openning a non-existing file in read mode should error:\n");
	fd1 = mini_file_open(fs, "file1.txt", false);
	score(fd1 == NULL);
	printf("\n");

	printf("Openning a non-existing file in write mode should work.\n");
	fd1 = mini_file_open(fs, "file1.txt", true);
	score(fd1 != NULL);

	printf("Openning an existing file in write mode (again) should error:\n");
	fd2 = mini_file_open(fs, "file1.txt", true);
	score(fd2 == NULL);
	printf("\n");

	printf("Openning an existing file in read mode (again) should work.\n");
	fd2 = mini_file_open(fs, "file1.txt", false);
	score(fd2 != NULL);

	printf("Openning an existing file in read mode (again, 2nd time) should work.\n");
	fd3 = mini_file_open(fs, "file1.txt", false);
	score(fd3 != NULL);

	printf("Openning 2nd, non-existing file in write mode should work.\n");
	fd4 = mini_file_open(fs, "file2.txt", true);
	score(fd4 != NULL);

	printf("Openning 2nd, non-existing file in write mode (again) should error:\n");
	fd5 = mini_file_open(fs, "file2.txt", true);
	score(fd5 == NULL);
	printf("\n");

	printf("Closing 2nd file should work.\n");
	score(mini_file_close(fs, fd4));

	printf("Reopenning 2nd file in write mode should work.\n");
	fd4 = mini_file_open(fs, "file2.txt", true);
	score(fd4 != NULL);

	printf("Openning 3rd, non-existing file in write mode should work.\n");
	fd6 = mini_file_open(fs, "file3.txt", true);
	score(fd6 != NULL);

	score(mini_file_close(fs, fd1));
	score(mini_file_close(fs, fd2));
	score(mini_file_close(fs, fd3));
	score(mini_file_close(fs, fd4));
	score(mini_file_close(fs, fd5) == false);
	score(mini_file_close(fs, fd6));
}

void test_delete_file2(FAT_FILESYSTEM * fs) {
	FAT_OPEN_FILE *fd1, *fd2;
	// Deleting the second file:
	printf("Trying to delelete 'file1.txt' should fail, as it's open:\n");
	fd1 = mini_file_open(fs, "file1.txt", true);
	score(fd1 != NULL);
	score(mini_file_delete(fs, "file1.txt") == false);
	score(mini_file_close(fs, fd1));
	printf("\n");

	printf("Trying to delete 'file2.txt' should fail, as it's open:\n");
	fd2 = mini_file_open(fs, "file2.txt", true);
	score(fd2 != NULL);
	score(mini_file_delete(fs, "file2.txt") == false);
	printf("\n");

	printf("Closing handle to 'file2.txt' should work.\n");
	score(mini_file_close(fs, fd2));

	printf("Closing handle to 'file2.txt' again should fail:\n");
	score(mini_file_close(fs, fd2) == false);
	printf("\n");

	printf("Trying to delete 'file2.txt' should now work.\n");
	score(mini_file_delete(fs, "file2.txt"), 3);
}


void test_write_to_file1(FAT_FILESYSTEM * fs) {
	FAT_OPEN_FILE *fd1;
	// Create a long string (more than 1 block):
	char buffer[4096] = "";
	char num[5];
	for (int i=0; i<50; ++i) {
		num[0] = (i/10) + '0';
		num[1] = (i%10) + '0';
		num[2] = '.';
		num[3] = ' ';
		num[4] = 0;
		strcat(buffer, num);
		strcat(buffer, fox);
	}

	int written;

	printf("Writing 3 chunks of 45 bytes, all should fit in 1 block.\n");
	fd1 = mini_file_open(fs, "file1.txt", true);
	written = mini_file_write(fs, fd1, strlen(fox), fox);
	score(written == 45);
	score(mini_file_size(fs, "file1.txt") == 45, 2);

	written = mini_file_write(fs, fd1, strlen(fox), fox);
	score(written == 45);
	score(mini_file_size(fs, "file1.txt") == 45*2);

	written = mini_file_write(fs, fd1, strlen(fox), fox);
	score(written == 45);
	score(mini_file_size(fs, "file1.txt") == 45*3);

	printf("Writing 1 chunk of %d bytes, should fit in multiple block (new blocks).\n", (int)strlen(buffer));
	written = mini_file_write(fs, fd1, strlen(buffer), buffer);
	score(written == strlen(buffer), 3);
	score(mini_file_size(fs, "file1.txt") == 45*3+strlen(buffer), 2);

	printf("Writing another chunk of 45 bytes, should fit in the last block.\n");
	written = mini_file_write(fs, fd1, strlen(fox), fox);
	score(written == 45);
	score(mini_file_size(fs, "file1.txt") == 45*4+strlen(buffer));

	score(mini_file_close(fs, fd1));
}

void test_read_from_file1(FAT_FILESYSTEM * fs) {
	FAT_OPEN_FILE *fd1, *fd2;
	char buffer[4096];
	int read;

	printf("Reading 45 bytes from file.\n");

	memset(buffer, 0, sizeof(buffer));
	fd2 = mini_file_open(fs, "file1.txt", false);
	read = mini_file_read(fs, fd2, 45, buffer);
	score(read == 45);
	score(strcmp(buffer, fox) == 0, 2);

	printf("Reading another 45 bytes from file.\n");

	memset(buffer, 0, sizeof(buffer));
	read = mini_file_read(fs, fd2, 45, buffer);
	score(read == 45);
	score(strcmp(buffer, fox) == 0);
	// printf("\t[%d] %s\n", read, buffer);

	printf("Reading 1 byte from file.\n");
	memset(buffer, 0, sizeof(buffer));
	read = mini_file_read(fs, fd2, 1, buffer);
	score(strcmp(buffer, "T") == 0);

	printf("Reading the rest of the file.\n");
	memset(buffer, 0, sizeof(buffer));
	read = mini_file_read(fs, fd2, 4096, buffer);
	score(read == 2539); // There's nothing more to read.
	score(strcmp(buffer+strlen(buffer)-5, "dog.\n") == 0);


	printf("Attempting to read from an empty file.\n");
	fd1 = mini_file_open(fs, "file3.txt", false);
	read = mini_file_read(fs, fd1, 10, buffer);
	score(read == 0);

	mini_file_close(fs, fd1);
	mini_file_close(fs, fd2);

}

void test_seek(FAT_FILESYSTEM * fs) {
	FAT_OPEN_FILE *fd1, *fd2;
	char buffer[4096];
	int read;
	bool res;


	fd1 = mini_file_open(fs, "file1.txt", true);
	fd2 = mini_file_open(fs, "file1.txt", false);

	printf("Reading 45 bytes from beginning file.\n");
	memset(buffer, 0, sizeof(buffer));
	res = mini_file_seek(fs, fd2, 0, true); // Seek to start
	score(res);
	read = mini_file_read(fs, fd2, 45, buffer);
	score(read == 45);
	score(strcmp(buffer, fox) == 0);

	printf("Reading 45 bytes from beginning file again.\n");
	memset(buffer, 0, sizeof(buffer));
	res = mini_file_seek(fs, fd2, 0, true); // Seek to start
	score(res);
	read = mini_file_read(fs, fd2, 45, buffer);
	score(read == 45);
	score(strcmp(buffer, fox) == 0);

	printf("Seeking to negative.\n");
	memset(buffer, 0, sizeof(buffer));
	res = mini_file_seek(fs, fd2, -10, true); // Seek to start
	score(res == false);
	read = mini_file_read(fs, fd2, 45, buffer);
	score(read == 45);
	score(strcmp(buffer, fox) == 0);

	printf("Seeking to after file.\n");
	memset(buffer, 0, sizeof(buffer));
	res = mini_file_seek(fs, fd2, mini_file_size(fs, "file1.txt") + 1, true); // Seek to start
	score(res == false);

	printf("Seeking 45 bytes forward.\n");
	memset(buffer, 0, sizeof(buffer));
	// res = mini_file_seek(fs, fd2, 0, true); // Seek to start
	res = mini_file_seek(fs, fd2, -45, false);
	score(res == true);
	read = mini_file_read(fs, fd2, 45, buffer);
	printf("%s\n", buffer);
	score(read == 45);
	score(strcmp(buffer, fox) == 0);

	printf("Relative seek to negative.\n");
	res = mini_file_seek(fs, fd2, -90 -1, false);
	score(res == false);

	printf("Relative seek to after file.\n");
	res = mini_file_seek(fs, fd2, mini_file_size(fs, "file1.txt") - 90 + 1, false);
	score(res == false);

	printf("Seek to middle of file and overwrite.\n");
	res = mini_file_seek(fs, fd1, 45 + 4, true);
	score(res);
	int written = mini_file_write(fs ,fd1, 5, "slowy");
	score(written = 5);

	res = mini_file_seek(fs, fd1, -5, false);
	memset(buffer, 0, sizeof(buffer));
	read = mini_file_read(fs, fd1, 5, buffer);
	score(read == 5);
	score(strcmp(buffer, "slowy") == 0);

	res = mini_file_seek(fs, fd2, 45, true);
	memset(buffer, 0, sizeof(buffer));
	read = mini_file_read(fs, fd2, 45, buffer);
	score(strcmp(buffer, "The slowy brown fox jumps over the lazy dog.\n") == 0);

	mini_file_close(fs, fd1);
	mini_file_close(fs, fd2);
}

void test_suite(FAT_FILESYSTEM * fs) {
	test_open_3_files(fs);
	test_delete_file2(fs);

	test_write_to_file1(fs);
	test_read_from_file1(fs);

	test_seek(fs);

	mini_fat_dump(fs);
}


int main()
{
	printf("Creating a FAT filesystem:\n");
	FAT_FILESYSTEM * fs = mini_fat_create("fs1.fat", 1024, 10);

	test_small_filesystem(mini_fat_create("temp.fat", 128, 3)); // Only 3 blocks, 1 metadata, 2 files.

	test_suite(fs);

	if (current_score == total_score) {
		// Everything is working, now test save/load:
		printf("Saving the FAT filesystem.\n");
		score(mini_fat_save(fs), 6);

		printf("Loading the FAT filesystem.\n");
		FAT_FILESYSTEM *loaded_fs = mini_fat_load("fs1.fat");
		mini_fat_dump(loaded_fs);

		score(mini_file_delete(loaded_fs, "file1.txt"));
		test_suite(loaded_fs);
	} else {
		printf("Skipping save/load tests as other tests are not passing.\n");
	}


	printf("Final score: %d/%d\n", current_score/3*2, 100);
	return 0;
}

