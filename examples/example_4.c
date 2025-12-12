
#include <stdio.h>
#define ZERROR_IMPLEMENTATION
#define Z_SHORT_ERR 
#include "zerror.h"

zres validate_input(const char *key, int value) 
{
    ensure(key != NULL,  10, "Key cannot be NULL");
    ensure(value >= 0,   11, "Value must be positive");
    ensure(value <= 100, 12, "Value must be <= 100");
    
    return zres_ok();
}

ResInt calculate_score(const char *key, int raw_score) 
{
    check_into(ResInt, validate_input(key, raw_score));
    
    return ResInt_ok(raw_score * 2);
}

zres app_main(void) 
{
    int score = try_into(zres, calculate_score("player1", 50));
    printf("Score: %d\n", score);

    printf("Attempting invalid score...\n");
    int bad = try_into(zres, calculate_score("player2", 150));
    printf("Bad Score: %d\n", bad);

    return zres_ok();
}

int main(void) 
{
    return run(app_main());
}
