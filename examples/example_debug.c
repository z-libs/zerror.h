#include <stdio.h>

#define ZERROR_IMPLEMENTATION
#define Z_SHORT_ERR 
#define Z_ENABLE_TRACE
#define Z_DEBUG
#include "zerror.h"

ResInt parse_id(const char *input) 
{
    if (input == NULL) 
    {
        return ResInt_err(zerr_create(400, "Input is NULL"));
    }
    if (input[0] == 'X') 
    {
        return ResInt_err(zerr_create(400, "Invalid ID format: '%s'", input));
    }
    return ResInt_ok(100);
}

zres process_user(const char *raw_id) 
{
    int id = try_into(zres, parse_id(raw_id));
    
    if (id < 1000) 
    {
        check_ctx(zres_err(zerr_create(500, "ID too low")), "Processing ID: %d", id);
    }

    return zres_ok();
}

zres app_main(void) 
{
    check(process_user("X123"));

    return zres_ok();
}

int main(void) 
{
    return run(app_main());
}
