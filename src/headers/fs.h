#pragma once
#include <types.h>

// TAR
struct tar_header 
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
};



//

int get_number_of_files(uint64_t tar_addr);