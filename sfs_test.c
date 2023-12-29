#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sfs_api.h"

int main() {
    mksfs(1);
    int f = sfs_fopen("some_name.txt");
    char my_data[] = "I just wrote this into the 'disk' of my small file system";
    char out_data[1024];
    sfs_fwrite(f, my_data, sizeof(my_data)+1);
    sfs_fseek(f, 0);
    sfs_fread(f, out_data, sizeof(out_data)+1);
    printf("%s\n", out_data);
    sfs_fclose(f);
    sfs_remove("some_name.txt");
}
