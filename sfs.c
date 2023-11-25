void mksfs(int fresh); // creates the file system
int sfs_getnextfilename(char *fname); // get the name of the next file in directory
int sfs_getfilesize(const char* path); // get the size of the given file
int sfs_fopen(char *name); // opens the given file
int sfs_fclose(int fileID); // closes the given file
int sfs_fwrite(int fileID, char *buf, int length); // write buf characters into disk
int sfs_fread(int fileID, char *buf, int length); // read characters from disk into buf
int sfs_fseek(int fileId, int loc); // seek to the location from beginning
int sfs_remove(char *file); // removes a file from the filesystem

void mksfs(int fresh)
{
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