
#include <stdio.h>
#define ZERROR_IMPLEMENTATION
#define ZERROR_SHORT_NAMES 
#include "zerror.h"

ResInt get_system_time(void) 
{
    // We simulate a critical system failure.
    return ResInt_err(zerr_create(500, "System Clock Unreachable"));
}

int main(void) 
{
    printf("Starting critical system...\n");

    // => unwrap()
    // Tries to get the value. If it's an error, it prints the error and ABORTS.
    // Use this when you are 100% sure it should work, or you don't care if it crashes.
    /* int t = unwrap(get_system_time()); */ 

    // => expect()
    // Same as unwrap, but lets you add a custom message to the panic output.
    // This provides more context during debugging.
    int t = expect(get_system_time(), "Fatal: Could not initialize timer subsystem");

    printf("System Time: %d\n", t);
    return 0;
}
