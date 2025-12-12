# zerror.h

`zerror.h` brings **Result Types** (like Rust's `Result<T, E>`) and robust error propagation to C. It replaces fragile integer error codes with a type-safe, expressive, and context-rich error handling system.

It is designed to be **Zero Overhead** in the success path and seamlessly integrates with GCC/Clang extensions to provide modern syntax (like `try()`) without breaking standard C compatibility.

## Features

* **Result Pattern**: Functions return a tagged union (`zres` or `ResInt`) containing either a value or an error.
* **Stack Tracing**: Errors automatically record their path through the call stack (Logical Traceback) when enabled.
* **Context Propagation**: Attach variables and messages to errors as they bubble up (for example, "Transaction failed" -> "ID: 420").
* **Debugger Integration**: Automatically triggers breakpoints (`SIGTRAP`) at the exact moment an error is created.
* **Modern Syntax**: Use `try()`, `check()`, and `defer()` macros for cleaner control flow.
* **Formatted Errors**: Create errors with `printf`-style formatting directly.
* **OS Integration**: Built-in support for wrapping system `errno` (for example, "Permission denied") and checking system calls.
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

### 1. Defining a Result Type

Instead of returning `int` and checking for `-1`, define a Result type for your data.

```c
// Defines "ResUser", "ResUser_ok", and "ResUser_err".
typedef struct { int id; char *name; } User;
DEFINE_RESULT(User, ResUser)
```

### 2. Returning Results

Functions return either `_ok(value)` or `_err(error)`. You can use formatted strings when creating errors.

```c
ResUser find_user(int id) 
{
    if (id < 0) 
    {
        // Z_DEBUG will trigger a breakpoint here automatically!
        return ResUser_err(zerr_create(400, "Invalid ID: %d", id));
    }
    return ResUser_ok((User){id, "Alice"});
}
```

### Handling Errors (The Modern Way)

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

## Debugging & Tracing

`zerror.h` is designed to make debugging C code significantly easier.

### Enable Tracing
Define `Z_ENABLE_TRACE` before including the header. Every time an error passes through `try` or `check`, the location is recorded.

```c
#define Z_ENABLE_TRACE
#include "zerror.h"
```

> **Output Example:**
> ```text
> [!] Error: Mass cannot be negative
>     at calculate_physics (physics.c:15)
>     at update_entity (game.c:24)
>     at game_loop (main.c:40) [Origin]
>     [Expr] calculate_physics(mass)
> ```

### Enable Debug Traps
Define `Z_DEBUG` to force a hardware breakpoint (`SIGTRAP`) whenever an error is created. This allows you to inspect the state in GDB/VS/LLDB *before* the stack unwinds.

```c
#define Z_DEBUG
#include "zerror.h"
```

## Advanced Usage

### Contextual Variables (`check_ctx`)

You can attach runtime values to the stack trace as the error bubbles up.

```c
zres process_transaction(int id) 
{
    // If this fails, the error trace will include "Transaction ID: [id]".
    check_ctx(verify_funds(id), "Transaction ID: %d", id);
    return zres_ok();
}
```

### System Errors (`zerr_errno`)

Use `zerr_errno` to automatically append the OS error string.

```c
FILE *f = fopen("secrets.txt", "r");
if (!f) 
{
    // Output: "Failed to open config: Permission denied".
    return ResPtr_err(zerr_errno(EACCES, "Failed to open config"));
}
```

### Wrapping Errors (`check_wrap`)

Wrap low-level errors with high-level context descriptions.

```c
zres load_texture(const char *path) 
{
    // If read_file fails, the error becomes:
    // "Failed to load sprite"
    //   | context: "File not found"
    check_wrap(read_file(path), "Failed to load sprite");
    return zres_ok();
}
```

### Validation (`ensure`)

Validate conditions without repetitive if-statements.

```c
ResInt calculate(int val) 
{
    ensure_into(ResInt, val > 0, 101, "Value must be positive");
    return ResInt_ok(val * 2);
}
```

### Resource Cleanup (`defer`)

Ensure resources are freed even if an error occurs.

```c
void process_file(const char* path) 
{
    FILE *f = fopen(path, "r");
    defer( fclose(f) ); // Runs when scope exits (Error OR Success).

    check( read_header(f) );
    // ...
}
```

## API Reference

**Core Macros (Flow Control)**

These macros rely on compiler extensions (Statement Expressions). They are available on GCC, Clang, and MSVC (recent versions).

| Macro | Description |
| :--- | :--- |
| `try(expr)` | Evaluates `expr`. If error, returns it. Else evaluates to value. **Use when return types match.** |
| `check(expr)` | Evaluates `expr` (void result). If error, returns it. |
| `try_into(T, expr)` | Like `try`, but converts the error to return type `T`. **Use when return types differ.** |
| `check_into(T, expr)`| Like `check`, but returns error of type `T`. |
|`check_sys(expr, fmt...)`| Checks integer `expr != 0`. If true, captures errno and returns error. |
| `check_ctx(expr, fmt...)`| Like `check`, but wraps the error with a formatted message (variables). |
| `check_wrap(expr, fmt...)`| Like `check`, but wraps the error with a new high-level description. |
| `try_ptr(T, ptr, code, msg)` | Checks if `ptr` is NULL. If so, returns error `T`. |
| `try_or(expr, default)` | Evaluates `expr`. If error, returns `default`. |
| `unwrap(expr)` | Panics (aborts) if `expr` is an error. |
| `expect(expr, msg)` | Like `unwrap`, but prints a custom message. |
| `defer(code)` | Executes `code` when the current scope exits (RAII). |

**Validation**

| Macro | Description |
| :--- | :--- |
| `ensure(cond, code, msg)` | If `cond` is false, returns `zres` error. |
| `ensure_into(T, cond, code, msg)` | If `cond` is false, returns `T` error. |

**Error Creation**

| Function/Macro | Description |
| :--- | :--- |
| `zerr_create(code, fmt, ...)` | Creates error with formatted message. Traps if `Z_DEBUG` is set. |
| `zerr_errno(code, fmt, ...)` | Like create, but appends `strerror(errno)`. |
| `zres_ok()` | Returns generic "Success". |
| `zres_err(e)` | Returns generic "Failure". |
| `T##_ok(val)` | Returns typed success (e.g., `ResInt_ok(5)`). |
| `T##_err(e)` | Returns typed failure. |

## Predefined Results

`zerror.h` comes with common result types pre-defined for convenience.

| Type | Result Name |
| :--- | :--- |
| `int` | `ResInt` |
| `float` | `ResFloat` |
| `bool` | `ResBool` |
| `size_t` | `ResSize` |
| `void*` | `ResPtr` |
| `char*` | `ResStr` |

## Configuration Options

| Define | Effect |
| :--- | :--- |
| `ZERROR_IMPLEMENTATION` | Enables the implementation (define in one `.c` file). |
| `Z_SHORT_ERR` | Enables short aliases (`try`, `check`, `defer`, etc.). |
| `Z_ENABLE_TRACE` | Enables logical stack tracing. |
| `Z_DEBUG` | Enables breakpoints (`SIGTRAP`) on error creation. |
| `Z_NO_COLOR` | Disables ANSI color codes in output. |
| `Z_PANIC_ACTION` | Override the panic behavior (Default: `abort()`). |