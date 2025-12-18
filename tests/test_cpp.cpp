
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>

#define ZERROR_IMPLEMENTATION
#define ZERROR_SHORT_NAMES
#include "zerror.h"

using namespace z_error;

#define TEST(name) printf("[TEST] %-40s", name);
#define PASS() std::cout << "\033[0;32mPASS\033[0m\n";

result<int> helper_calc(int val) 
{
    if (val == 0) 
    {
        return zerr_create(101, "Zero not allowed");
    }
    return val * 10;
}

result<std::string> helper_str(bool fail) 
{
    if (fail) 
    {
        return zerr_create(500, "String generation failed");
    }
    return std::string("Hello World");
}

void test_result_basics() 
{
    TEST("Result<T> Basics (Ok, Err)");

    // Success.
    result<int> r1 = 42;
    assert(r1.ok());
    assert(r1.unwrap_val() == 42);

    // Failure.
    result<int> r2 = zerr_create(404, "Not Found");
    assert(!r2.ok());
    assert(r2.err.code == 404);
    assert(std::string(r2.err.msg) == "Not Found");

    // Implicit bool conversion.
    if (r1) { /* OK */ } else { assert(false); }
    if (!r2) { /* OK */ } else { assert(false); }

    PASS();
}

void test_result_void() 
{
    TEST("Result<void>");

    // Use named factory 'success()' or default ctor
    result<void> r1 = result<void>::success();
    assert(r1.ok());

    result<void> r2 = zerr_create(500, "Fail");
    assert(!r2.ok());

    PASS();
}

void test_complex_types() 
{
    TEST("Complex Types (std::vector, Ptr)");

    // Vector (move/copy).
    std::vector<int> data = {1, 2, 3};
    result<std::vector<int>> r_vec = data;
    
    assert(r_vec.ok());
    assert(r_vec.unwrap_val().size() == 3);
    assert(r_vec.unwrap_val()[2] == 3);

    // Unique ptr (move only).
    std::unique_ptr<int> ptr(new int(99));
    result<std::unique_ptr<int>> r_ptr = std::move(ptr);

    assert(r_ptr.ok());
    assert(*r_ptr.unwrap_val() == 99);

    PASS();
}

// Requires statement expressions (GCC/Clang/TCC).
#if defined(__GNUC__) || defined(__clang__)
void test_macros_cpp() 
{
    TEST("Macros C++ (ztry, check)");

    auto func = [](int input) -> result<std::string> 
    {
        int val = ztry(helper_calc(input));
        
        // ensure works via zres implicit conversion.
        ensure(val < 100, 200, "Value too high");

        return std::to_string(val);
    };

    // Success path.
    auto r1 = func(5); 
    assert(r1.ok());
    assert(r1.unwrap_val() == "50");

    // Propagated error.
    auto r2 = func(0);
    assert(!r2.ok());
    assert(r2.err.code == 101); 

    // Ensure error.
    auto r3 = func(20); 
    assert(!r3.ok());
    assert(r3.err.code == 200); 

    PASS();
}
#endif

void test_implicit_conversion() 
{
    TEST("Implicit Conversion (zres -> Result)");

    // C-style function returning zres.
    auto c_func = []() -> zres { return zres_err(zerr_create(1, "C Error")); };

    // Assigned to C++ Result.
    result<void> r = c_func();
    assert(!r.ok());
    assert(r.err.code == 1);

    PASS();
}

int main() 
{
    std::cout << "=> Running tests (zerror.h, cpp).\n";
    
    test_result_basics();
    test_result_void();
    test_complex_types();
    test_implicit_conversion();

#if defined(__GNUC__) || defined(__clang__)
    test_macros_cpp();
#endif

    std::cout << "=> All tests passed successfully.\n";
    return 0;
}

