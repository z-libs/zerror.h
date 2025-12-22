
#include <stdio.h>

#define ZTIME_IMPLEMENTATION
#include "ztime.h"

#define ZERROR_IMPLEMENTATION
#include "zerror.h"

ResInt perform_calculation(int input) 
{
    if (input < 0) 
    {
        return ResInt_err(zerr_create(-1, "Input cannot be negative"));
    }
    return ResInt_ok(input * 2);
}

int main(void) 
{
    char time_buf[64];
    ztime_fmt_log(time_buf, sizeof(time_buf), (time_t)ztime_epoch());
    printf("[%s] Application Started\n", time_buf);

    ResInt result1 = perform_calculation(10);
    if (result1.is_ok) 
    {
        printf("Result 1: %d\n", result1.val);
    } 
    else 
    {
        zerr_print(result1.err);
    }

    printf("\nTriggering error.\n");
    ResInt result2 = perform_calculation(-5);
    if (!result2.is_ok) 
    {
        zerr_print(result2.err);
    }

    printf("\nTriggering panic.\n");
    zerr_panic("Critical system failure", __FILE__, __LINE__);

    return 0;
}
