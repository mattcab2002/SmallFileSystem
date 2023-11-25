#include "disk_emu.h"

#define true 1
#define false 0
#define MAX_FILE_NAME_LENGTH 16
#define BLOCK_SIZE 1024
#define NUM_BLOCKS 8

void mksfs(int fresh); // creates the file system
int sfs_getnextfilename(char *fname); // get the name of the next file in directory
int sfs_getfilesize(const char* path); // get the size of the given file
int sfs_fopen(char *name); // opens the given file
int sfs_fclose(int fileID); // closes the given file
int sfs_fwrite(int fileID, char *buf, int length); // write buf characters into disk
int sfs_fread(int fileID, char *buf, int length); // read characters from disk into buf
int sfs_fseek(int fileId, int loc); // seek to the location from beginning
int sfs_remove(char *file); // removes a file from the filesystem

//------------------------------- Structs -------------------------------//

typedef struct inode {
    int mode;
    int link_cnt;
    int uid;
    int gid;
    int size;
    int d_pointers; // direct pointers
    int s_in_pointer; // single indirect pointer
} inode;

typedef struct super_block {
    int magic_num;
    int block_size;
    int file_system_size;
    int inode_table_l;
    inode root_dir;
} super_block;

typedef struct inode_table
{
    inode[] inodes;
    int free_inodes;
    int earliest_available;
} inode_t;

typedef struct data_blocks {

} dbs;

typedef struct free_bit_map {

} fbm;

typedef struct on_disk_data_struct {
    super_block sb;
    inode_t inode_table;
    int data_blocks;
    fbm bit_map;
} on_disk;

typedef struct directory {
    char filename[MAX_FILE_NAME_LENGTH];
    inode inode;
} dir;


// typedef struct in_memory_data_struct {} in_mem;

// in_mem open_fd_table;
// in_mem dir_table;
// in_mem disk_block_cache;
// in_mem inode_cache;

//------------------------------- Globals -------------------------------//

inode_t inode_table;

//------------------------------- Helpers -------------------------------//

void init_inode_table() {
    inode_table.earliest_available = 0;
    inode_table.free_inodes = 0;
}

int init_inode() {
    if (inode_table.free_inodes == 0) {
        perror("No remaining space in file system.")
    }
}

//------------------------------- Api Methods -------------------------------//

void mksfs(int fresh)
{
    if (!fresh) { // load from storage
        
    } else {
        init_fresh_disk("fs.sfs", BLOCK_SIZE, NUM_BLOCKS);
    }
}

int sfs_getnextfilename(char *fname)
{
    return 0;
}

int sfs_getfilesize(const char *path)
{
    return 0;
}

int sfs_fopen(char *name)
{
    return 0;
}

int sfs_fclose(int fileID)
{
    return 0;
}

int sfs_fwrite(int fileID, char *buf, int length)
{
    return 0;
}

int sfs_fread(int fileID, char *buf, int length)
{
    return 0;
}

int sfs_fseek(int fileId, int loc)
{
    return 0;
}

int sfs_remove(char *file)
{
    return 0;
}

int main() {
    return 1;
}