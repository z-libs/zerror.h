
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define ZERROR_IMPLEMENTATION
#define ZERROR_SHORT_NAMES
#include "zerror.h"

#define TEST(name) printf("[TEST] %-35s", name);
#define PASS() printf(" \033[0;32mPASS\033[0m\n")

zres helper_success(void) 
{
    return zres_ok();
}

zres helper_fail(void) 
{
    return zres_err(zerr_create(500, "Internal Server Error"));
}

ResInt helper_typed_val(int x) 
{
    if (x < 0) return ResInt_err(zerr_create(400, "Negative Value"));
    return ResInt_ok(x);
}

zres flow_check_proxy(void) 
{
    check(helper_fail());
    return zres_ok();
}

ResInt flow_try_proxy(int x) 
{
    // If error, return ResInt error. If ok, unwrap int.
    int val = try_into(ResInt, helper_typed_val(x)); 
    return ResInt_ok(val * 2);
}

// Helper for testing validation (must return zres to use ensure).
zres helper_validation(int x) 
{
    ensure(x > 0, 10, "Must be positive");
    return zres_ok();
}

void test_creation(void) 
{
    TEST("Creation (Basic, Errno)");

    // zerr_create.
    zerr e1 = zerr_create(404, "Not Found: %s", "index.html");
    assert(e1.code == 404);
    assert(strstr(e1.msg, "index.html") != NULL);
    assert(e1.line != 0);
    assert(e1.file != NULL);

    // zerr_errno.
    zerr e2 = zerr_errno(EACCES, "File Locked");
    assert(e2.code == EACCES);
    assert(strstr(e2.msg, "File Locked") != NULL);
    
    // Check that it captured the system errno string.
    assert(strlen(e2.msg) > 11); 

    PASS();
}

void test_flow_macros(void) 
{
    TEST("Flow (Check, Try_Into)");

    // check().
    zres r1 = flow_check_proxy();
    assert(!r1.is_ok);
    assert(r1.err.code == 500);
    assert(strstr(r1.err.msg, "Internal Server Error") != NULL);

    // try_into() -> Success.
    ResInt r2 = flow_try_proxy(10);
    assert(r2.is_ok);
    assert(r2.val == 20);

    // try_into() -> Failure propagation.
    ResInt r3 = flow_try_proxy(-5);
    assert(!r3.is_ok);
    assert(r3.err.code == 400);

    PASS();
}

void test_validation(void) 
{
    TEST("Ensure / Validation");

    // Case 1: Pass.
    zres r1 = helper_validation(10);
    assert(r1.is_ok);

    // Case 2: Fail.
    zres r2 = helper_validation(-5);
    assert(!r2.is_ok);
    assert(r2.err.code == 10);
    
    PASS();
}

void test_wrapping(void) 
{
    TEST("Error Wrapping");

    zerr root = zerr_create(503, "Database Unavailable");
    zerr wrapped = zerr_wrap(root, "Login Failed");

    assert(wrapped.code == 503);
    assert(strstr(wrapped.msg, "Login Failed") != NULL);
    assert(strstr(wrapped.msg, "Database Unavailable") != NULL);
    assert(strstr(wrapped.msg, "context") != NULL);

    PASS();
}

// Extension test (GCC/Clang only).
#if defined(__GNUC__) || defined(__clang__)
void test_defer(void) 
{
    TEST("Defer (Cleanup)");
    
    int flag = 0;
    {
        defer( flag = 1; );
        assert(flag == 0);
    } // defer executes here.
    
    assert(flag == 1);
    PASS();
}
#endif

int main(void) 
{
    printf("=> Running tests (zerror.h, main).\n");
    
    test_creation();
    test_flow_macros();
    test_wrapping();
    test_validation();

#if defined(__GNUC__) || defined(__clang__)
    test_defer();
#endif

    printf("=> All tests passed successfully.\n");
    return 0;
}

