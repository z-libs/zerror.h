
#include <stdio.h>
#include <errno.h>
#define ZERROR_IMPLEMENTATION
#define Z_SHORT_ERR 
#include "zerror.h"

ResPtr open_config_file(const char *path) 
{
    FILE *f = fopen(path, "r");
    if (f == NULL) 
    {
        return ResPtr_err(zerr_errno(errno, "Could not open config at '%s'", path));
    }
    return ResPtr_ok(f);
}

zres app_main(void) 
{
    printf("Attempting to open missing file...\n");

    FILE *f = try_into(zres, open_config_file("ghost_settings.ini"));

    check_sys(fclose(f), "Failed to close config file");

    return zres_ok();
}

int main(void) 
{
    return run(app_main());
}
