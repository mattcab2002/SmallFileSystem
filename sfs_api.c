#include "disk_emu.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define true 1
#define false 0
#define MAX_FILE_NAME_LENGTH 16
#define BLOCK_SIZE 1024
#define MAX_DIRECTORIES 20
#define FD_TABLE_SIZE 20
#define NUM_BLOCKS 1024 // 1 MB file system
#define NUM_POINTERS 13

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
    int d_pointer[12]; // direct pointers
    int in_pointer;    // single indirect pointer
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
    int earliest_available;
    int *map;
} fbm;

typedef struct on_disk_data_struct
{
    super_block sb;
    inode_t inode_table;
    int data_blocks;
    fbm bit_map;
} on_disk;

//---------------------------- Memory Structs ----------------------------//

typedef struct open_fdt_entry
{
    inode_s inode;
    int offset;
    int fd;
} fdt_entry;

typedef struct open_fd_table
{
    int earliest_available;
    fdt_entry *table;
} fd_table;

fd_table open_fd_table;
dir_e *dir_cache = NULL;

//------------------------------- Globals -------------------------------//

const inode_s default_inode = {
    .mode = 0,
    .link_cnt = 0,
    .uid = -1,
    .gid = 0,
    .size = 0,
    .d_pointer = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    .in_pointer = -1};

const dir_e default_dir = {.filename = "", .inode = {.mode = 0, .link_cnt = 0, .uid = -1, .gid = 0, .size = 0, .d_pointer = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, .in_pointer = -1}};

const fdt_entry default_fdt_entry = {.fd = -1, .offset = -1, .inode = {.mode = 0, .link_cnt = 0, .uid = -1, .gid = 0, .size = 0, .d_pointer = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, .in_pointer = -1}};

inode_t inode_table;
super_block sb;
fbm bit_map; // map of free data blocks
int num_entries = 0;
int dir_index = 0;
char empty_block[BLOCK_SIZE];

//------------------------------- Helpers -------------------------------//

void init_empty_block()
{
    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        empty_block[i] = '\0';
    }
}

void print(char *message)
{
    printf("%s\n", message);
}

void init_free_bit_map()
{
    bit_map.earliest_available = 0;
    int map[BLOCK_SIZE] = {}; // will initialize values to 0
    bit_map.map = map;
}

int get_blocks_available()
{
    int counter = 0;
    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        if (bit_map.map[i] == 0)
        {
            counter++;
        }
    }
    return counter;
}

void init_open_fd_table()
{
    fdt_entry *table = malloc(FD_TABLE_SIZE * sizeof(fdt_entry)); // will initialize values to 0

    for (int i = 0; i < FD_TABLE_SIZE; i++)
    {
        table[i] = default_fdt_entry;
    }
    open_fd_table.table = table;
}

void init_super_block()
{
    dir_e *dirs = malloc(MAX_DIRECTORIES * sizeof(dir_e));
    for (int i = 0; i < MAX_DIRECTORIES; i++)
    {
        dirs[i] = default_dir;
    };
    sb.magic_num = 0;
    sb.block_size = BLOCK_SIZE;
    sb.file_system_size = 0;
    sb.inode_table_l = 0;
    sb.root_dir = dirs;
}

void init_dir_cache()
{
    dir_cache = sb.root_dir;
}

void add_mapping_to_super_block(dir_e entry)
{
    sb.root_dir[num_entries] = entry;
}

void remove_mapping_from_super_block(dir_e entry)
{
    for (int i = 0; i < MAX_DIRECTORIES; i++)
    {
        dir_e entry = sb.root_dir[i];
        if (strcmp(entry.filename, entry.filename) == 0)
        {
            sb.root_dir[i] = default_dir;
            break;
        }
    }
}

int add_mapping(dir_e entry)
{
    if (num_entries > MAX_DIRECTORIES)
    {
        print("File system full.");
        return -1;
    }
    add_mapping_to_super_block(entry);
    if (dir_cache != NULL)
    {
        dir_cache = sb.root_dir; // instance of root_dir
    }
    num_entries += 1;
    return 1;
}

int remove_mapping(dir_e entry)
{
    if (num_entries == 0)
    {
        print("File system empty. Nothing to remove.");
        return -1;
    }
    remove_mapping_from_super_block(entry);
    if (dir_cache != NULL)
    {
        dir_cache = sb.root_dir; // instance of root_dir
    }
    num_entries -= 1;
    return 1;
}

void init_inode_table()
{
    inode_table.earliest_available = 0;
    inode_table.free_inodes = MAX_DIRECTORIES;
    inode_table.length = 0;
    inode_s inodes[MAX_DIRECTORIES] = {};
    for (int i = 0; i < MAX_DIRECTORIES; i++)
    {
        inodes[i] = default_inode;
    }
    inode_table.inodes = inodes;
}

int create_inode_entry(inode_s new_node)
{
    if (inode_table.length == MAX_DIRECTORIES)
    {
        print("Cannot add anymore inodes to the table");
        return -1;
    }
    for (int i = 0; i < MAX_DIRECTORIES; i++)
    {
        if (inode_table.inodes[i].uid == -1)
        { // if default then replace
            inode_table.inodes[i] = new_node;
            inode_table.free_inodes--;
            inode_table.length++;
            return 1;
        }
    }
    return -1;
}

inode_s remove_inode(int uid)
{
    inode_s node;
    if (inode_table.free_inodes == MAX_DIRECTORIES)
    {
        print("No inodes to remove.");
        return default_inode;
    }
    for (int i = 0; i < MAX_DIRECTORIES; i++)
    {
        node = inode_table.inodes[i];
        if (node.uid == uid)
        {
            inode_table.inodes[i] = default_inode;
            inode_table.length--;
            inode_table.free_inodes++;
            return node;
        }
    }
    return default_inode;
}

inode_s init_inode()
{
    if (inode_table.free_inodes == 0)
    {
        print("No remaining space in file system.");
        return default_inode;
    }
    inode_s new_node = default_inode;
    new_node.uid = inode_table.length;
    inode_table.length++;
    return new_node;
}

dir_e get_dir_entry(char *filename)
{
    for (int i = 0; i < MAX_DIRECTORIES; i++)
    {
        dir_e entry = dir_cache[i];
        if (entry.filename == filename)
        {
            return entry;
        }
    }
    return default_dir;
}

int update_dir_entry(inode_s node)
{
    for (int i = 0; i < MAX_DIRECTORIES; i++)
    {
        dir_e entry = dir_cache[i];
        if (entry.inode.uid == node.uid)
        {
            entry.inode = node;
            dir_cache[i] = entry;
            return 1;
        }
    }
    return 0;
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

inode_s get_inode(int uid)
{
    int index;
    if ((index = get_inode_index(uid)) != -1)
    {
        return inode_table.inodes[index];
    }
    else
    {
        return default_inode;
    }
}

inode_s delete_inode(int uid)
{
    int table_length = inode_table.length;
    if (table_length == 0)
    {
        print("No inodes to delete");
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
    print("Inode not found");
    return default_inode;
}

int does_file_exist(char *name)
{
    if (dir_cache == NULL)
    {
        print("Directory cache not initialized. Please set.");
        return -1;
    }
    for (int i = 0; i < MAX_DIRECTORIES; i++)
    {
        if (strcmp(dir_cache[i].filename, name) == 0)
        {
            return true;
        }
    }
    return false;
}

int create_file(char *name, inode_s new_node)
{
    if (does_file_exist(name))
    {
        print("File with same name already exists");
        return 1;
    }

    if (strlen(name) > MAX_FILE_NAME_LENGTH)
    {
        print("File name too long");
        return -1;
    }

    dir_e inode_dir;
    inode_dir.filename = name;
    inode_dir.inode = new_node;
    // update root directory in super block and directory cache
    add_mapping(inode_dir);
    return 1;
}

int does_fd_exist(int fd)
{
    for (int i = 0; i < FD_TABLE_SIZE; i++)
    {
        if (open_fd_table.table[i].fd == fd)
        {
            return open_fd_table.table[i].inode.uid;
        }
    }
    return false;
}

fdt_entry get_fd_entry(int fd)
{
    for (int i = 0; i < FD_TABLE_SIZE; i++)
    {
        if (open_fd_table.table[i].fd == fd)
        {
            return open_fd_table.table[i];
        }
    }
    return default_fdt_entry;
}

int update_fd_entry(fdt_entry entry)
{
    for (int i = 0; i < FD_TABLE_SIZE; i++)
    {
        if (open_fd_table.table[i].fd == entry.fd)
        {
            open_fd_table.table[i] = entry;
            return 1;
        }
    }
    return 0;
}

int create_fd_entry(inode_s node)
{
    if (open_fd_table.earliest_available > FD_TABLE_SIZE)
    {
        print("Max number of open file descriptors reached. Please close one in order to continue.");
        return -1;
    }
    fdt_entry new_entry;
    new_entry.fd = open_fd_table.earliest_available;
    new_entry.offset = 0; // file descriptor to end of the file
    new_entry.inode = node;
    open_fd_table.table[open_fd_table.earliest_available] = new_entry;
    for (int i = 0; i < FD_TABLE_SIZE; i++)
    {
        if (open_fd_table.table[i].fd == -1)
        {
            open_fd_table.earliest_available = i;
            break;
        }
    }
    return new_entry.fd;
}

int delete_fd_entry(int fd)
{
    for (int i = 0; i < FD_TABLE_SIZE; i++)
    {
        fdt_entry entry = open_fd_table.table[i];
        if (entry.fd == fd)
        {
            if (open_fd_table.earliest_available > i)
            {
                open_fd_table.earliest_available = i;
            }
            open_fd_table.table[i] = default_fdt_entry;
            return 0;
        }
    }
    print("File descriptor for node does not exist.");
    return -1;
}

int *allocate_blocks(int bytes, int *blocks_written)
{
    int blocks_needed = bytes / BLOCK_SIZE + (bytes % BLOCK_SIZE != 0); // round up in case of imperfect division
    if (blocks_needed > NUM_POINTERS)
    {
        print("Blocks needed > 13");
        return NULL;
    }
    int blocks_available = get_blocks_available();
    if (blocks_needed > blocks_available)
    {
        print("Do not have enough blocks left to support allocation.");
        return NULL;
    }
    int *blocks_allocated = (int *)malloc(blocks_needed);
    int counter = 0;
    for (int i = 0; i < BLOCK_SIZE && counter != blocks_needed; i++)
    {
        if (bit_map.map[i] == 0)
        {
            blocks_allocated[counter] = i;
            counter++;
            bit_map.map[i] = 1;
        }
    }
    *blocks_written = blocks_needed;
    return blocks_allocated;
}

int count_num_blocks(inode_s node)
{
    int counter = 0;
    for (int i = 0; i < NUM_POINTERS; i++)
    {
        if (i == NUM_POINTERS - 1)
        {
            if (node.in_pointer != -1)
            {
                counter++;
                break;
            }
        }
        if (node.d_pointer[i] != -1)
        {
            counter++;
        }
    }
    return counter;
}

int *get_blocks(inode_s node, int *num_blocks)
{
    int counter = count_num_blocks(node);
    int *blocks_allocated = (int *)malloc(counter * sizeof(int));
    for (int i = 0; i < NUM_POINTERS && i != counter; i++)
    {
        if (i == NUM_POINTERS - 1)
        {
            if (node.in_pointer != -1)
            {
                blocks_allocated[i] = node.in_pointer;
                break;
            }
        }

        if (node.d_pointer[i] != -1)
        {
            blocks_allocated[i] = node.d_pointer[i];
        }
    }
    *num_blocks = counter;
    return blocks_allocated;
}

int release_blocks(int *blocks, int size)
{
    for (int i = 0; i < size; i++)
    {
        int block = blocks[i];
        if (block < 0)
        {
            print("Unexpected block");
            return -1;
        }
        bit_map.map[blocks[i]] = 0;

        write_blocks(block, 1, empty_block);
    }
    return 1;
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
    init_empty_block();
    init_free_bit_map();
    init_inode_table();
    init_open_fd_table();
    init_super_block();
    init_dir_cache();
}

int sfs_getnextfilename(char *fname)
{
    if (dir_index == -1)
    {
        print("No more files in directory.");
        return -1;
    }
    strcpy(fname, dir_cache[dir_index].filename);
    for (int i = dir_index + 1; i < MAX_DIRECTORIES; i++)
    {
        dir_e entry = dir_cache[i];
        if (entry.inode.uid != -1)
        {
            dir_index = i;
            return 1;
        }
    }
    dir_index = -1; // if gets to here no more entries
    return 0;
}

int sfs_getfilesize(const char *path)
{
    for (int i = 0; i < MAX_DIRECTORIES; i++)
    {
        dir_e entry = dir_cache[i];
        if (strcmp(path, entry.filename) == 0)
        {
            return entry.inode.size;
        }
    }
    print("File not found");
    return -1;
}

int sfs_fopen(char *name)
{
    int fd;
    inode_s new_node = init_inode();
    if (new_node.uid == -1)
    { // default inode
        print("Probleming initializing inode.");
        return -1;
    }
    new_node.size = 0; // file size
    if (create_file(name, new_node) == -1 || (fd = create_fd_entry(new_node)) == -1 || create_inode_entry(new_node) == -1)
    {
        print("SFS Failed to open file.");
        return -1;
    }
    if (dir_index == -1)
    { // send dir_entry to this entry
    }
    return fd;
}

int sfs_fclose(int fileID)
{
    return delete_fd_entry(fileID);
}

int sfs_fwrite(int fileID, char *buf, int length)
{
    fdt_entry entry;
    if ((entry = get_fd_entry(fileID)).fd == -1)
    {
        print("File entry does not exist. Please consider creating it.");
        return -1;
    }
    inode_s inode = entry.inode;
    int blocks_written;
    int *blocks = allocate_blocks(sizeof(char) * length, &blocks_written);
    if (blocks == NULL)
    {
        print("Was unable to allocate blocks for file write");
        return -1;
    }
    int counter = 0;
    for (int i = 0; i < NUM_POINTERS && counter != blocks_written; i++)
    {
        int set = 0;
        for (int j = 0; j < NUM_POINTERS - 1; j++)
        {
            if (inode.d_pointer[j] == -1)
            {
                inode.d_pointer[j] = blocks[i];
                set = 1;
                counter++;
                break;
            }
        }
        if (set)
        {
            continue;
        }
        if (inode.in_pointer != -1)
        {
            inode.in_pointer = blocks[i];
            counter++;
        }
    }
    for (int i = 0; i < blocks_written; i++)
    {
        char *to_write = (char *)malloc(BLOCK_SIZE);
        strncpy(to_write, buf + (i * BLOCK_SIZE), BLOCK_SIZE); // copy a certain substring up to a given length
        write_blocks(blocks[i], 1, to_write);                  // flush to disk
    }
    free(blocks);
    entry.offset = entry.offset + blocks_written;
    entry.inode = inode;
    update_fd_entry(entry);
    update_dir_entry(entry.inode);
    return blocks_written * BLOCK_SIZE;
}

int sfs_fread(int fileID, char *buf, int length)
{
    fdt_entry entry;
    if ((entry = get_fd_entry(fileID)).fd == -1)
    {
        print("File entry does not exist. Please consider creating it.");
        return -1;
    }
    inode_s inode = entry.inode;
    int num_blocks;
    int *blocks = get_blocks(inode, &num_blocks);
    int start_block = entry.offset;
    for (int i = start_block; i < num_blocks; i++)
    {
        read_blocks(blocks[i], 1, buf);
    }
    free(blocks);
    return (num_blocks - start_block) * BLOCK_SIZE;
}

int sfs_fseek(int fileId, int loc)
{
    fdt_entry entry = get_fd_entry(fileId);
    if (entry.inode.uid == -1) // receieved default entry
    {
        print("INode with fileId not found");
        return -1;
    }
    entry.offset = loc;
    update_fd_entry(entry);
    return 1;
}

int sfs_remove(char *file)
{
    dir_e entry = get_dir_entry(file);
    if (entry.inode.uid == -1)
    { // received default
        print("File set for removal not found");
        return -1;
    }
    int result = remove_mapping(entry);
    inode_s node = remove_inode(entry.inode.uid);
    if (node.uid == -1)
    { // received default inode
        print("Unable to delete inode");
        return -1;
    }
    int counter = count_num_blocks(entry.inode);
    int *blocks_to_be_released = (int *)malloc(counter * sizeof(int));
    for (int i = 0; i < NUM_POINTERS || i < counter; i++)
    {
        if (i == NUM_POINTERS - 1)
        {
            if (entry.inode.in_pointer != -1)
            {
                blocks_to_be_released[i] = entry.inode.in_pointer;
                break;
            }
        }
        if (entry.inode.d_pointer[i] != -1)
        {
            blocks_to_be_released[i] = entry.inode.d_pointer[i];
        }
    }
    release_blocks(blocks_to_be_released, counter);
    return 1;
}
