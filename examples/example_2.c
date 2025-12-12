
#include <stdio.h>
#include <stdlib.h>

#define ZERROR_IMPLEMENTATION
#define Z_SHORT_ERR 
#include "zerror.h"

zres verify_header(FILE *f) 
{
    char buffer[4];
    if (fread(buffer, 1, 4, f) != 4) 
    {
        return zres_err(zerr_create(2, "File too short"));
    }
    return zres_err(zerr_create(3, "Invalid Magic Bytes"));
}

zres process_file(const char *path) 
{
    FILE *f = fopen(path, "w+");
    ensure(f != NULL, 1, "Could not open file");
    
    defer( fclose(f); printf("[Debug] File closed.\n"); );

    fprintf(f, "BAD");
    rewind(f);

    printf("Verifying file...\n");

    // Won't be nice.
    check( verify_header(f) );

    printf("File processed.\n");
    return zres_ok();
}

int main(void) 
{
    return run(process_file("test.tmp"));
}
