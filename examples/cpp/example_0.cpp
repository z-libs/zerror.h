#include <iostream>

#define ZERROR_IMPLEMENTATION
#define ZERROR_SHORT_NAMES
#define ZERROR_ENABLE_TRACE 
#include "zerror.h"

using namespace z_error; // Use the C++ namespace.

// Returns a Result containing a float OR an Error.
result<float> safe_div(float a, float b) 
{
    if (b == 0.0f) 
    {
        // Implicit conversion from error -> result<float>.
        return error(101, "Division by zero");
    }
    return a / b;
}

int main() 
{
    auto res1 = safe_div(10.0f, 2.0f);
    if (res1.ok()) 
    {
        std::cout << "10 / 2 = " << res1.val << "\n";
    }

    auto res2 = safe_div(10.0f, 0.0f);
    if (!res2.ok()) 
    {
        // z_error::error works with std::ostream natively.
        std::cout << "Failed: " << res2.err << "\n";
    }

    return 0;
}
