#include <fs.h>
// FS implementation

// tar fs driver


unsigned int getsize(const char *in)
{
 
    unsigned int size = 0;
    unsigned int j;
    unsigned int count = 1;
 
    for (j = 11; j > 0; j--, count *= 8)
        size += ((in[j - 1] - '0') * count);
 
    return size;
 
}

struct tar_header *headers[32];

unsigned int parse(uint64_t address)
{
 
    unsigned int i;
 
    for (i = 0; ; i++)
    {
 
        struct tar_header *header = (struct tar_header *)address;
 
        if (header->name[0] == '\0')
            break;
 
        unsigned int size = getsize(header->size);
 
        headers[i] = header;
 
        address += ((size / 512) + 1) * 512;
 
        if (size % 512)
            address += 512;
 
    }
 
    return i;
 
}


int get_number_of_files(uint64_t address) {
    return parse(address);
}

