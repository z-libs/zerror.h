
#include <stdio.h>

#define ZERROR_IMPLEMENTATION
#define Z_SHORT_ERR 
#include "zerror.h"

zres read_sector(int sector_id) 
{
    if (sector_id > 1000) 
    {
        return zres_err(zerr_create(500, "Hardware Read Error"));
    }
    return zres_ok();
}

zres load_file(const char *filename) 
{
    printf("Loading %s...\n", filename);
    check_wrap(read_sector(9999), "Failed to read file header");
    return zres_ok();
}

zres app_main(void) 
{
    check( load_file("data.db") );
    return zres_ok();
}

int main(void) 
{
    return run(app_main());
}
