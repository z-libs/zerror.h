
#include <stdio.h>
#include <string.h>

#define ZERROR_IMPLEMENTATION
#define Z_SHORT_ERR 
#include "zerror.h"

typedef struct 
{
    int id;
    char username[32];
    bool is_admin;
} User;

DEFINE_RESULT(User, ResUser)

ResUser find_user_by_id(int id) 
{
    if (id < 0) 
    {
        return ResUser_err(zerr_create(400, "Invalid ID format"));
    }
    if (id == 0) 
    {
        return ResUser_err(zerr_create(404, "User not found"));
    }

    User u = { .id = id, .is_admin = (id == 1) };
    snprintf(u.username, 32, "User_%d", id);
    
    return ResUser_ok(u);
}

zres app_logic(void) 
{
    User admin = try_into(zres, find_user_by_id(1));
    printf("Found Admin: %s\n", admin.username);

    // This will fail.
    User ghost = try_into(zres, find_user_by_id(0));
    printf("Should not see this: %s\n", ghost.username);

    return zres_ok();
}

int main(void) 
{
    return run(app_logic());
}
