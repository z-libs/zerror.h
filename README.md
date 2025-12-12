# zerror.h

`zerror.h` brings **Result Types** (like Rust's `Result<T, E>`) and robust error propagation to C. It replaces fragile integer error codes and global `errno` checks with a type-safe, expressive, and context-rich error handling system.

It is designed to be **Zero Overhead** in the success path and seamlessly integrates with GCC/Clang extensions to provide modern syntax (like `try()`) without breaking standard C compatibility.

## Features

* **Result Pattern**: Functions return a tagged union (`zres` or `ResInt`) containing either a value or an error.
* **Context Propagation**: Errors carry file, line, and source context automatically.
* **Zero Boilerplate**: Pre-defined results for common types (`int`, `bool`, `size_t`, `void*`).
* **Modern Syntax**: Use `try()`, `check()`, and `defer()` macros for cleaner control flow (requires GCC/Clang/MSVC extensions).
* **Error Wrapping**: Wrap low-level errors with high-level context (for example, "File Not Found" -> "Config Load Failed").
* **Thread Safe**: Error buffers use Thread Local Storage (TLS).
* **Single Header**: Drop-in and use. No linking required.

## Installation

1.  Copy `zerror.h` to your project's include folder.
2.  Include it where needed.

```c
#define ZERROR_IMPLEMENTATION // Define in ONE .c file.
#include "zerror.h"
```

> **Optional Short Macros:** To enable short keywords like `try`, `check`, and `defer`, define `Z_SHORT_ERR` before including the header.

```c
#define Z_SHORT_ERR
#include "zerror.h"
```

## Quick Start

### Defining a Result Type

Instead of returning `int` and checking for `-1`, define a Result type for your data.

```c
// Defines "ResUser", "ResUser_ok", and "ResUser_err".
typedef struct { int id; char *name; } User;
DEFINE_RESULT(User, ResUser)
```

### Returning Results

Functions return either `_ok(value)` or `_err(error)`.

```c
ResUser find_user(int id) 
{
    if (id < 0) 
    {
        return ResUser_err(zerr_create(1, "Invalid ID"));
    }
    return ResUser_ok((User){id, "Alice"});
}
```

### Handling Errors

Use `try()` to propagate errors automatically.

```c
ResUser login(int id) 
{
    // If find_user fails, 'try' returns the error immediately.
    // If it succeeds, 'u' gets the value.
    User u = try(find_user(id));
    
    printf("Welcome, %s!\n", u.name);
    return ResUser_ok(u);
}
```

## Usage: Architecture

`zerror.h` encourages a specific architectural pattern:

1.  **Leaf Functions** return typed results (`ResInt`, `ResPtr`).
2.  **Logic Functions** propagate errors using `try()` / `check()`.
3.  **Entry Point (Main)** catches and prints the final error using `run()`.

### The "Main" Wrapper

It is recommended to wrap your logic in a function returning `zres` (void result) and call it from `main`.

```c
#include "zerror.h"

// Your logic entry point
zres app_main(void) {
    int config = try_into(zres, load_config("settings.ini"));
    check( process_data(config) );
    return zres_ok();
}

int main(void) {
    // 'run' executes the function and pretty-prints any unhandled error.
    return run(app_main());
}
```

## API Reference

**Core Macros (Flow Control)**

These macros rely on compiler extensions (Statement Expressions). They are available on GCC, Clang, and MSVC (recent versions).

| Macro | Description |
| :--- | :--- |
| `try(expr)` | Evaluates `expr`. If it returns an error, it is returned immediately. Otherwise, evaluates to the internal value. **Use when return types match.** |
| `check(expr)` | Evaluates `expr` (which must return `zres`). If it fails, the error is returned. **Use for void-returning functions.** |
| `try_into(T, expr)` | Evaluates `expr`. If it fails, converts the error to type `T` and returns it. **Use when return types differ.** |
| `check_into(T, expr)`| Like `check`, but for functions returning `T` instead of `zres`. |
| `try_ptr(T, ptr, code, msg)` | Checks if `ptr` is NULL. If so, returns an error of type `T`. Otherwise, returns `ptr`. |
| `try_or(expr, default)` | Evaluates `expr`. If it fails, evaluates to `default`. If it succeeds, evaluates to the value. |
| `unwrap(expr)` | Panics (aborts) if `expr` is an error. Useful for prototyping or tests. |
| `expect(expr, msg)` | Like `unwrap`, but prints a custom message before panicking. |
| `defer(code)` | Executes `code` when the current scope exits (RAII). |

**Error Creation**

| Function/Macro | Description |
| :--- | :--- |
| `zerr_create(code, msg)` | Creates a new error with the current file/line captured. |
| `zres_ok()` | Returns a generic "Success" result (void). |
| `zres_err(e)` | Returns a generic "Failure" result wrapping `e`. |
| `T##_ok(val)` | Returns a typed success result (for example, `ResInt_ok(5)`). |
| `T##_err(e)` | Returns a typed failure result (for example, `ResInt_err(e)`). |

## Error Context & Wrapping

You can wrap errors to add context as they bubble up the stack. This creates a "Traceback" like effect without the overhead of capturing stack traces.

```c
zres load_texture(const char *path) 
{
    FILE *f = fopen(path, "rb");
    if (!f) return zres_err(zerr_create(1, "File not found"));
    // ...
    return zres_ok();
}

zres init_player() 
{
    // If load_texture fails, the error becomes: 
    // "Failed to load player sprite"
    //   | context: "File not found"
    check(zerr_wrap(load_texture("hero.png"), "Failed to load player sprite"));
    return zres_ok();
}
```

## Predefined Results

`zerror.h` comes with common result types pre-defined for convenience.

| Type | Result Name | Ok Constructor | Err Constructor |
| :--- | :--- | :--- | :--- |
| `int` | `ResInt` | `ResInt_ok(v)` | `ResInt_err(e)` |
| `float` | `ResFloat` | `ResFloat_ok(v)` | `ResFloat_err(e)` |
| `bool` | `ResBool` | `ResBool_ok(v)` | `ResBool_err(e)` |
| `size_t` | `ResSize` | `ResSize_ok(v)` | `ResSize_err(e)` |
| `void*` | `ResPtr` | `ResPtr_ok(v)` | `ResPtr_err(e)` |
| `char*` | `ResStr` | `ResStr_ok(v)` | `ResStr_err(e)` |

To define your own, use:

```c
DEFINE_RESULT(MyType, ResName)
```

## Compilation & Extensions

`zerror.h` is strictly Standard C compliant, but the "Flow Control Macros" (`try`, `check`, `defer`) rely on **Statement Expressions**, a widely supported compiler extension.

* **Supported:** GCC, Clang, Intel C, MSVC (experimental/recent).
* **Fallback:** If `Z_HAS_MODERN_C` detects an unsupported environment, these macros are disabled. You can still use the `if (res.is_ok)` pattern manually.

## C++ Interop

`zerror.h` is C++ compatible (`extern "C"`). However, since C++ has exceptions, you might not need `zerror` in pure C++ code. It is primarily designed for C codebases or C++ projects interacting with C interfaces.

The result types are generated as standard structs/unions, making them POD (Plain Old Data) and safe to pass across C/C++ boundaries.