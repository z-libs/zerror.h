
#include <stdio.h>

#define ZERROR_IMPLEMENTATION
#define Z_SHORT_ERR 
#include "zerror.h"

ResInt safe_div(int a, int b) 
{
    if (b == 0) 
    {
        return ResInt_err(zerr_create(101, "Division by zero"));
    }
    return ResInt_ok(a / b);
}

zres app_main(void) 
{
    int result = try_into(zres, safe_div(10, 2));
    printf("10 / 2 = %d\n", result);

    int bad = try_into(zres, safe_div(10, 0));
    printf("This line is never reached! %d\n", bad);

    return zres_ok();
}

int main(void) 
{
    return run(app_main());
}
