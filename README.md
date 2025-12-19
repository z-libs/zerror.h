# zerror.h

`zerror.h` is a sophisticated, single-header error handling library for C and C++. It bridges the gap between low-level C programming and modern safety patterns found in languages like Rust or Swift by providing Result types, stack traces, and "Logical Stack Traces" through macro-driven decoration.

It is part of the [Zen Development Kit](https://github.com/z-libs/z-libs).

## Features

* **Type-Safe Results**: Generic `result<T>` patterns for both C (via macros) and C++.
* **Logical Stack Traces**: Capture file, line, and function names at the point of origin and throughout the propagation chain.
* **Zero-Allocation Printing**: Uses thread-local ring buffers for error message formatting to avoid heap fragmentation.
* **C++ Support**: Native C++11 wrapper with RAII `result<T>` and `std::ostream` integration.
* **Modern C Ergonomics**: Leverages `__attribute__((cleanup))` and statement expressions for `try`-like syntax on supported compilers.
* **Debug Integration**: Optional hardware breakpoints/traps (`ZERROR_TRAP`) when an error is created.

## Usage: C

Define result types and use the propagation macros to handle errors without boilerplate.

```c
#define ZERROR_IMPLEMENTATION
#include "zerror.h"

// Define a Result type that can return an int or a zerr.
DEFINE_RESULT(int, ResInt)

ResInt calculate(int val) 
{
    if (val < 0) 
    {
        // Creates error with file/line/func context.
        return ResInt_err(zerr_create(Z_EINVAL, "Value cannot be negative: %d", val));
    }
    return ResInt_ok(val * 2);
}

zres process() 
{
    // 'try' automatically unwraps value or returns error to caller.
    int x = try(calculate(-5)); 
    printf("Result: %d\n", x);
    return zres_ok();
}

int main() 
{
    // 'run' executes the logic and prints a formatted trace on failure.
    return run(process());
}
```

## Usage: C++

The C++ wrapper lives in the **`z_error`** namespace and provides a template-based `result<T>` that behaves similarly to `std::expected`.

```c
#include <iostream>
#define ZERROR_IMPLEMENTATION
#include "zerror.h"

z_error::result<int> compute(int n) 
{
    if (n == 0) 
    {
        return zerr_create(Z_ERR, "Division by zero");
    }
    return 100 / n;
}

int main()
{
    auto res = compute(0);
    if (!res) 
    {
        std::cerr << "Caught error: " << res.err << std::endl;
        return 1;
    }
    std::cout << "Value: " << res.unwrap_val() << std::endl;
    return 0;
}
```

## API Reference (C)

**Error Creation & Management**

| Macro / Function | Description |
| :--- | :--- |
| `zerr_create(code, fmt, ...)` | Creates a `zerr` struct with context and a formatted message. |
| `zerr_errno(code, fmt, ...)` | Like `zerr_create`, but appends the current `strerror(errno)`. |
| `zerr_print(e)` | Prints a stylized, color-coded error report to `stderr`. |
| `zerr_panic(msg, file, line)` | Prints a panic message and calls `abort()`. |
| `zres_ok()` / `zres_err(e)` | Helper constructors for the basic `zres` (void result) type. |

**Flow Control**

| Macro | Description |
| :--- | :--- |
| `try(expr)` | Evaluates `expr`. If error, returns it from current function. Otherwise returns the value. |
| `try_into(Type, expr)` | Same as `try`, but converts the error to a different Result type before returning. |
| `check(expr)` | Propagates error for functions returning `zres`. |
| `check_ctx(expr, fmt, ...)` | Propagates error but appends a "Context" string to the message. |
| `unwrap(expr)` | Returns value or panics/traps if `expr` is an error. |
| `defer(code)` | Executes `code` when the current scope exits (RAII for C). |

## Short Names (Opt-In)

If you prefer a cleaner API and don't have naming conflicts, define `ZERROR_SHORT_NAMES` before including the header. This allows you to use aliases like `try` and `check` instead of their prefixed counterparts.

| Short Name | Original Macro |
| :--- | :--- |
| `try(e)` | `ZERROR_TRY(e, #e)` |
| `check(e)` | `ZERROR_CHECK(e, #e)` |
| `check_ctx(e, ...)` | `ZERROR_CHECK_CTX(e, #e, ...)` |
| `ensure(cond, code, msg)` | `ZERROR_ENSURE(cond, #cond, code, msg)` |
| `unwrap(e)` | `ZERROR_EXPECT(e, "unwrap() failed")` |
| `defer(code)` | `zerr_defer(code)` (via `Z_HAS_CLEANUP`) |

> **Note:** In C++, `try` is a reserved keyword. The library automatically provides `ztry` as the short name instead when compiled as C++.

## API Reference (C++)

**`class z_error::result<T>`**

| Method | Description |
| :--- | :--- |
| `ok()` / `operator bool()` | Returns `true` if the result contains a value. |
| `unwrap_val()` | Returns the value or panics if an error is present. |
| `err` | Public member containing the `zerr` struct on failure. |
| `operator zerr()` | Explicit conversion to the underlying C error type. |

**`class z_error::error`**

A thin C++ wrapper around the `zerr` struct for easier integration with C++ streams.

## Configuration

Define these macros **before** including `zerror.h` to modify behavior:

| Macro | Description |
| :--- | :--- |
| `ZERROR_IMPLEMENTATION` | Define in **exactly one** `.c` or `.cpp` file to generate logic. |
| `ZERROR_SHORT_NAMES` | Enables shorter aliases (for example, `try`, `check`, `ensure`). |
| `ZERROR_DEBUG` | Enables hardware traps/breakpoints at the exact moment an error is created. |
| `ZERROR_ENABLE_TRACE` | Enables the collection of propagation traces. |
| `ZERROR_NO_COLOR` | Disables ANSI color codes in `zerr_print`. |
| `ZERROR_PANIC_ACTION` | Define to override the default `abort()` behavior. |

## Memory Management

`zerror.h` uses an internal thread-local ring buffer to handle error messages. This allows you to return formatted error messages from functions without worrying about `malloc` or `free`.

The common library (`zcommon.h` block) also allows you to override the standard allocators used by the system.