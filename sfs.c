#include "disk_emu.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define true 1
#define false 0
#define MAX_FILE_NAME_LENGTH 16
#define BLOCK_SIZE 1024
#define NUM_BLOCKS 8

void mksfs(int fresh);                             // creates the file system
int sfs_getnextfilename(char *fname);              // get the name of the next file in directory
int sfs_getfilesize(const char *path);             // get the size of the given file
int sfs_fopen(char *name);                         // opens the given file
int sfs_fclose(int fileID);                        // closes the given file
int sfs_fwrite(int fileID, char *buf, int length); // write buf characters into disk
int sfs_fread(int fileID, char *buf, int length);  // read characters from disk into buf
int sfs_fseek(int fileId, int loc);                // seek to the location from beginning
int sfs_remove(char *file);                        // removes a file from the filesystem

//------------------------------- Structs -------------------------------//

typedef struct inode
{
    int mode;
    int link_cnt;
    int uid;
    int gid;
    int size;
    int d_pointers;   // direct pointers
    int s_in_pointer; // single indirect pointer
} inode_s;

typedef struct directory_entry
{
    char *filename;
    inode_s inode;
} dir_e;

typedef struct super_block
{
    int magic_num;
    int block_size;
    int file_system_size;
    int inode_table_l;
    dir_e *root_dir;
} super_block;

typedef struct inode_table
{
    int free_inodes;
    int earliest_available;
    int length;
    inode_s *inodes;
} inode_t;

typedef struct data_blocks
{

} dbs;

typedef struct free_bit_map
{

} fbm;

typedef struct on_disk_data_struct
{
    super_block sb;
    inode_t inode_table;
    int data_blocks;
    fbm bit_map;
} on_disk;

// typedef struct in_memory_data_struct {} in_mem;

// in_mem open_fd_table;
// in_mem dir_table;
// in_mem disk_block_cache;
// in_mem inode_cache;

//------------------------------- Globals -------------------------------//

const inode_s default_inode = {
    .mode = 0,
    .link_cnt = 0,
    .uid = -1,
    .gid = 0,
    .size = 0,
    .d_pointers = 0,
    .s_in_pointer = 0};

const dir_e default_dir = {
    "",
    {.mode = 0,
     .link_cnt = 0,
     .uid = -1,
     .gid = 0,
     .size = 0,
     .d_pointers = 0,
     .s_in_pointer = 0}};

inode_t inode_table;
super_block sb;

//------------------------------- Helpers -------------------------------//

void init_super_block()
{
    dir_e dirs[12] = {};
    for (int i = 0; i < sizeof(dirs) / sizeof(dir_e); i++)
    {
        dirs[i] = default_dir;
    };
    sb.magic_num = 0;
    sb.block_size = BLOCK_SIZE;
    sb.file_system_size = 0;
    sb.inode_table_l = 0;
    sb.root_dir = dirs;
}

void init_inode_table()
{
    inode_table.earliest_available = 0;
    inode_table.free_inodes = 0;
    inode_table.length = 0;
    inode_s inodes[12] = {};
    for (int i = 0; i < sizeof(inodes) / sizeof(inode_s); i++)
    {
        inodes[i] = default_inode;
    }
    inode_table.inodes = inodes;
}

inode_s init_inode()
{
    if (inode_table.free_inodes == 0)
    {
        perror("No remaining space in file system.");
    }
    inode_s new_node;
    new_node.mode = 0;
    new_node.link_cnt = 0;
    new_node.uid = inode_table.length;
    new_node.gid = 0;
    new_node.size = 0;
    new_node.d_pointers = 0;
    new_node.s_in_pointer = 0;
    return new_node;
}

int get_inode_index(int uid)
{
    for (int i = 0; i < inode_table.length; i++)
    {
        inode_s curr = inode_table.inodes[i];
        if (curr.uid == uid)
        {
            return i;
        }
    }
    return -1;
}

inode_s delete_inode(int uid)
{
    int table_length = inode_table.length;
    if (table_length == 0)
    {
        perror("No inodes to delete");
    }

    int node_index;
    if ((node_index = get_inode_index(uid)) != -1)
    {
        inode_s curr = inode_table.inodes[node_index];
        inode_table.inodes[node_index] = default_inode;
        if (node_index < inode_table.earliest_available)
        {
            inode_table.earliest_available = node_index;
        }
        return curr;
    }
    perror("Inode not found");
    return default_inode;
}

//------------------------------- Api Methods -------------------------------//

void mksfs(int fresh)
{
    srand((unsigned int)(time(0))); // random number generator
    if (!fresh)
    { // load from storage
        init_disk("fs.sfs", BLOCK_SIZE, NUM_BLOCKS);
    }
    else
    {
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
    if (strlen(name) > MAX_FILE_NAME_LENGTH)
    {
        perror("File name too large");
        return -1;
    }
    inode_s new_node = init_inode();
    dir_e inode_dir;
    inode_dir.filename = name;
    inode_dir.inode = new_node;
    // update root directory in super block and directory cache
    return 1;
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

int main()
{
    return 1;
}