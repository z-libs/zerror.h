
#include <stdio.h>
#define ZERROR_IMPLEMENTATION
#define ZERROR_SHORT_NAMES 
#define ZERROR_ENABLE_TRACE 
#include "zerror.h"

ResInt calculate_physics(int mass) 
{
    if (mass < 0) 
    {
        return ResInt_err(zerr_create(500, "Mass cannot be negative"));
    }
    return ResInt_ok(mass * 10);
}

zres update_entity(int id, int mass) 
{
    int force = try_into(zres, calculate_physics(mass));
    
    printf("Entity %d updated with force %d\n", id, force);
    return zres_ok();
}

zres game_loop(void) 
{
    printf("Tick 1...\n");
    check(update_entity(1, 50));
    
    printf("Tick 2 (Error Incoming)...\n");
    check(update_entity(2, -5));

    return zres_ok();
}

int main(void) 
{
    return run(game_loop());
}
