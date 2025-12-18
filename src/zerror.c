
/*
 * zerror.h — Error handling, result types, and panic machinery
 * Part of Zen Development Kit (ZDK)
 *
 * A single-header library for robust error handling in C and C++.
 *
 * Usage:
 * #define ZERROR_IMPLEMENTATION
 * #include "zerror.h"
 *
 * License: MIT
 * Author: Zuhaitz
 * Repository: https://github.com/z-libs/zerror.h
 * Version: 1.1.0
 */

#ifndef ZERROR_H
#define ZERROR_H

#include "zcommon.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h> 

#ifdef __cplusplus
#   include <iostream>
#   include <string>
#   include <stdexcept>
#   include <type_traits>
#   include <utility>
#   include <new>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Config.

#ifndef ZERROR_PANIC_ACTION
#   define ZERROR_PANIC_ACTION() abort()
#endif

// Base types

typedef struct 
{
    int code;
    const char *msg;
    const char *file;
    int line;
    const char *func;
    const char *source;
} zerr;

zerr zerr_create_impl(int code, const char *file, int line, const char *func, const char *fmt, ...);
zerr zerr_errno_impl(int code, const char *file, int line, const char *func, const char *fmt, ...);
zerr zerr_add_trace(zerr e, const char *func, const char *file, int line);
zerr zerr_wrap(zerr e, const char *fmt, ...);
void zerr_print(zerr e);
void zerr_panic(const char *msg, const char *file, int line);

// Result types.

#define DEFINE_RESULT(T, Name)                                                              \
    typedef struct { bool is_ok; union { T val; zerr err; }; } Name;                        \
    static inline Name Name##_ok(T v) { Name r; r.is_ok = true; r.val = v; return r; }      \
    static inline Name Name##_err(zerr e) { Name r; r.is_ok = false; r.err = e; return r; }

typedef struct 
{ 
    bool is_ok; 
    zerr err; 
} zres;

static inline zres zres_ok(void)
{ 
    return (zres){ .is_ok = true, .err = {0, NULL, NULL, 0, NULL, NULL} }; 
}

static inline zres zres_err(zerr e) 
{ 
    return (zres){ .is_ok = false, .err = e }; 
}

DEFINE_RESULT(int,      ResInt)
DEFINE_RESULT(float,    ResFloat)
DEFINE_RESULT(double,   ResDouble)
DEFINE_RESULT(bool,     ResBool)
DEFINE_RESULT(size_t,   ResSize)
DEFINE_RESULT(void*,    ResPtr)
DEFINE_RESULT(char*,    ResStr)

#if defined(ZERROR_DEBUG) && defined(_MSC_VER)
#   define ZERROR_TRAP() __debugbreak()
#elif defined(ZERROR_DEBUG)
#   define ZERROR_TRAP() __builtin_trap()
#else
#   define ZERROR_TRAP() ((void)0)
#endif

#define zerr_create(code, ...) (ZERROR_TRAP(), zerr_create_impl((code), __FILE__, __LINE__, __func__, __VA_ARGS__))
#define zerr_errno(code, ...) (ZERROR_TRAP(), zerr_errno_impl((code), __FILE__, __LINE__, __func__, __VA_ARGS__))

#ifdef ZERROR_ENABLE_TRACE
#   define ZERROR_TRACE_OP(e) zerr_add_trace(e, __func__, __FILE__, __LINE__)
#else
#   define ZERROR_TRACE_OP(e) (e)
#endif

// Macros.

// Thread local storage detection
#if defined(__cplusplus) && __cplusplus >= 201103L
#   define ZERROR_THREAD_LOCAL thread_local
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#   define ZERROR_THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
#   define ZERROR_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__TINYC__) || defined(__clang__)
#   define ZERROR_THREAD_LOCAL __thread
#else
#   define ZERROR_THREAD_LOCAL
#endif

// Macro generation.
#define ZERROR_UID(prefix) Z_CONCAT(prefix, __LINE__)

static inline zerr zerr_with_src(zerr e, const char *src) 
{
    if (NULL == e.source) 
    {
        e.source = src;
    }
    return e;
}

// Runtime helper.

int zerr_run(zres result);

#define ZERROR_RUN(expr) zerr_run(expr)

// Universal macros.

// Check condition, return error if false.
#define ZERROR_ENSURE(cond, src, code, msg)         \
    do {                                            \
        if (!(cond))                                \
        {                                           \
            zerr _e = zerr_create((code), (msg));   \
            _e.source = src;                        \
            return zres_err(_e);                    \
        }                                           \
    } while(0)

// Check condition, return typed error if false.
#define ZERROR_ENSURE_INTO(RetType, cond, src, code, msg)   \
    do {                                                    \
        if (!(cond))                                        \
        {                                                   \
            zerr _e = zerr_create((code), (msg));           \
            _e.source = src;                                \
            return RetType##_err(_e);                       \
        }                                                   \
    } while(0)

// Check integer result from system call (0 is success).
#define ZERROR_CHECK_SYS(expr, fmt, ...)                            \
    do {                                                            \
        if ((expr) != 0)                                            \
        {                                                           \
            return zres_err(zerr_errno(errno, fmt, ##__VA_ARGS__)); \
        }                                                           \
    } while(0)


//  Modern macros (Require typeof and statement expressions).

#if Z_HAS_TYPEOF && (!defined(__cplusplus) || defined(__GNUC__))
    
    // Check result, propagate Error (void/zres return).
#   define ZERROR_CHECK(expr, src)                                              \
        do {                                                                    \
            Z_TYPEOF(expr) ZERROR_UID(_r) = (expr);                             \
            if (!ZERROR_UID(_r).is_ok)                                          \
            {                                                                   \
                ZERROR_UID(_r).err = zerr_with_src(ZERROR_UID(_r).err, src);    \
                ZERROR_UID(_r).err = ZERROR_TRACE_OP(ZERROR_UID(_r).err);       \
                return zres_err(ZERROR_UID(_r).err);                            \
            }                                                                   \
        } while(0)

    // Check result, propagate Error (typed return conversion).
#   define ZERROR_CHECK_INTO(RetType, expr, src)                                \
        do {                                                                    \
            Z_TYPEOF(expr) Z_UID(_r) = (expr);                                  \
            if (!ZERROR_UID(_r).is_ok)                                          \
            {                                                                   \
                ZERROR_UID(_r).err = zerr_with_src(ZERROR_UID(_r).err, src);    \
                ZERROR_UID(_r).err = ZERROR_TRACE_OP(ZERROR_UID(_r).err);       \
                return RetType##_err(ZERROR_UID(_r).err);                       \
            }                                                                   \
        } while(0)

    // Check result, wrap Error message.
#   define ZERROR_CHECK_WRAP(expr, src, fmt, ...)                                   \
        do {                                                                        \
            Z_TYPEOF(expr) ZERROR_UID(_r) = (expr);                                 \
            if (!ZERROR_UID(_r).is_ok)                                              \
            {                                                                       \
                ZERROR_UID(_r).err = zerr_with_src(ZERROR_UID(_r).err, src);        \
                ZERROR_UID(_r).err = ZERROR_TRACE_OP(ZERROR_UID(_r).err);           \
                return zres_err(zerr_wrap(ZERROR_UID(_r).err, fmt, ##__VA_ARGS__)); \
            }                                                                       \
        } while(0)
    
     // Check result, add context to Error message.
#   define ZERROR_CHECK_CTX(expr, src, fmt, ...)                                        \
        do {                                                                            \
            Z_TYPEOF(expr) ZERROR_UID(_r) = (expr);                                     \
            if (!ZERROR_UID(_r).is_ok)                                                  \
            {                                                                           \
                ZERROR_UID(_r).err = zerr_with_src(ZERROR_UID(_r).err, src);            \
                ZERROR_UID(_r).err = ZERROR_TRACE_OP(ZERROR_UID(_r).err);               \
                ZERROR_UID(_r).err = zerr_wrap(ZERROR_UID(_r).err, fmt, ##__VA_ARGS__); \
                return zres_err(ZERROR_UID(_r).err);                                    \
            }                                                                           \
        } while(0)

    // Try: evaluate, propagate Error if any, otherwise return value.
#   define ZERROR_TRY(expr, src)                                                    \
        ({  Z_TYPEOF(expr) ZERROR_UID(_res) = (expr);                               \
            if (!ZERROR_UID(_res).is_ok)                                            \
            {                                                                       \
                ZERROR_UID(_res).err = zerr_with_src(ZERROR_UID(_res).err, src);    \
                ZERROR_UID(_res).err = ZERROR_TRACE_OP(ZERROR_UID(_res).err);       \
                return ZERROR_UID(_res);                                            \
            }                                                                       \
            ZERROR_UID(_res).val;                                                   \
        })

    // Try into: evaluate, propagate Error converted to RetType, otherwise return value.
#   define ZERROR_TRY_INTO(RetType, expr, src)                                      \
        ({  Z_TYPEOF(expr) ZERROR_UID(_res) = (expr);                               \
            if (!ZERROR_UID(_res).is_ok)                                            \
            {                                                                       \
                ZERROR_UID(_res).err = zerr_with_src(ZERROR_UID(_res).err, src);    \
                ZERROR_UID(_res).err = ZERROR_TRACE_OP(ZERROR_UID(_res).err);       \
                return RetType##_err(ZERROR_UID(_res).err);                         \
            }                                                                       \
            ZERROR_UID(_res).val;                                                   \
        })
    
    // Try ptr: checks if NULL, returns Error if so.
#   define ZERROR_TRY_PTR(RetType, expr, src, code, msg)    \
        ({  Z_TYPEOF(expr) ZERROR_UID(_p) = (expr);         \
            if (NULL == ZERROR_UID(_p))                     \
            {                                               \
                zerr _e = zerr_create((code), (msg));       \
                _e.source = src;                            \
                return RetType##_err(_e);                   \
            }                                               \
            ZERROR_UID(_p);                                 \
        })

    // Try or: if error, return default.
#   define ZERROR_TRY_OR(expr, default_val)                                 \
        ({  Z_TYPEOF(expr) ZERROR_UID(_res) = (expr);                       \
            ZERROR_UID(_res).is_ok ? ZERROR_UID(_res).val : (default_val);  \
        })
    
    // Expect: evaluate, panic if error, otherwise return value.
#   define ZERROR_EXPECT(expr, msg)                         \
        ({  Z_TYPEOF(expr) ZERROR_UID(_res) = (expr);       \
            if (!ZERROR_UID(_res).is_ok)                    \
            {                                               \
                zerr_print(ZERROR_UID(_res).err);           \
                zerr_panic(msg, __FILE__, __LINE__);        \
            }                                               \
            ZERROR_UID(_res).val;                           \
        })

    // Defer (cleanup attribute).
#   define ZERROR_DEFER_HK(l, c)                                \
        void Z_CONCAT(z_defer_fn_, l)(void *_) { (void)_; c; }  \
        __attribute__((cleanup(Z_CONCAT(z_defer_fn_, l))))      \
        char Z_CONCAT(z_defer_var_, l)
    
#   define zerr_defer(code) ZERROR_DEFER_HK(__LINE__, code)

#else
    // Strict fallback (Standard C / MSVC).
    // Note: ZERROR_TRY macros are not supported here because C11 doesn't have 
    //       statement expressions. You must use CHECK_INTO manually.

#   define ZERROR_CHECK(expr, src)                                      \
        do {                                                            \
            zres ZERROR_UID(_r) = (expr);                               \
            if (!ZERROR_UID(_r).is_ok)                                  \
            {                                                           \
                return zres_err(ZERROR_TRACE_OP(ZERROR_UID(_r).err));   \
            }                                                           \
        } while(0)

#   define ZERROR_CHECK_INTO(RetType, expr, src)                        \
        do {                                                            \
            RetType ZERROR_UID(_r) = (expr);                            \
            if (!ZERROR_UID(_r).is_ok)                                  \
            {                                                           \
                return RetType##_err(ZERROR_TRACE_OP(Z_UID(_r).err));   \
            }                                                           \
        } while(0)
    
#   define ZERROR_CHECK_WRAP(expr, src, fmt, ...)                                   \
        do {                                                                        \
            zres ZERROR_UID(_r) = (expr);                                           \
            if (!ZERROR_UID(_r).is_ok)                                              \
            {                                                                       \
                return zres_err(zerr_wrap(ZERROR_UID(_r).err, fmt, ##__VA_ARGS__)); \
            }                                                                       \
        } while(0)
    
#   define ZERROR_CHECK_CTX(expr, src, fmt, ...)                                        \
        do {                                                                            \
            zres ZERROR_UID(_r) = (expr);                                               \
            if (!ZERROR_UID(_r).is_ok)                                                  \
            {                                                                           \
                ZERROR_UID(_r).err = zerr_wrap(ZERROR_UID(_r).err, fmt, ##__VA_ARGS__); \
                return zres_err(Z_UID(_r).err);                                         \
            }                                                                           \
        } while(0)

#endif

// Short names.
#ifdef ZERROR_SHORT_NAMES
#   define check(expr)                  ZERROR_CHECK(expr, #expr)
#   define check_into(T, expr)          ZERROR_CHECK_INTO(T, expr, #expr)
#   define check_wrap(expr, ...)        ZERROR_CHECK_WRAP(expr, #expr, __VA_ARGS__)
#   define check_ctx(expr, ...)         ZERROR_CHECK_CTX(expr, #expr, __VA_ARGS__)
#   define check_sys(expr, ...)         ZERROR_CHECK_SYS(expr, __VA_ARGS__)

#   define ensure(c, code, m)           ZERROR_ENSURE(c, #c, code, m)
#   define ensure_into(T, c, code, m)   ZERROR_ENSURE_INTO(T, c, #c, code, m)

#   ifdef ZERROR_TRY
        // "try" is a reserved keyword in C++.
#       ifdef __cplusplus
#           define ztry(expr)           ZERROR_TRY(expr, #expr)
#           define ztry_into(T, expr)   ZERROR_TRY_INTO(T, expr, #expr)
#           define ztry_ptr(T, p, c, m) ZERROR_TRY_PTR(T, p, #p, c, m)
#           define ztry_or(e, d)        ZERROR_TRY_OR(e, d)
#       else
#           define try(expr)            ZERROR_TRY(expr, #expr)
#           define try_into(T, expr)    ZERROR_TRY_INTO(T, expr, #expr)
#           define try_ptr(T, p, c, m)  ZERROR_TRY_PTR(T, p, #p, c, m)
#           define try_or(e, d)         ZERROR_TRY_OR(e, d)
#       endif

#       define unwrap(e)                ZERROR_EXPECT(e, "unwrap() failed")
#       define expect(e, m)             ZERROR_EXPECT(e, m)
#   endif

#   ifdef ZERROR_DEFER_HK
#       define defer(code)              zerr_defer(code)
#   endif
    
#   define run                          ZERROR_RUN(e)
#endif

#ifdef __cplusplus
} // extern "C"
#endif

// --- C++ Integration ---

#ifdef __cplusplus
inline std::ostream& operator<<(std::ostream& os, const zerr& e) 
{
    os << "[!] Error: " << (e.msg ? e.msg : "Unknown");
    if (e.file) 
    {
        os << " at " << e.file << ":" << e.line;
    }
    return os;
}

namespace z_error 
{
    class error 
    {
        ::zerr inner;
    public:
        error() : inner{0, nullptr, nullptr, 0, nullptr, nullptr} {}
        error(::zerr e) : inner(e) {}
        error(int code, const char *msg) : inner(::zerr_create_impl(code, nullptr, 0, nullptr, "%s", msg)) {}
        int code() const 
        { 
            return inner.code; 
        }

        operator ::zerr() const 
        { 
            return inner; 
        }

        friend std::ostream &operator<<(std::ostream &os, const error &e) 
        { 
            return os << e.inner; 
        }
    };

    // Generic Result<T> for C++.
    template <typename T>
    class result 
    {
    public:
        bool is_ok;
        union 
        { 
            T val; 
            ::zerr err; 
        };

        result(const T &value) : is_ok(true), val(value) {}
        result(T &&value) : is_ok(true), val(std::move(value)) {}
        result(::zerr e) : is_ok(false), err(e) {}
        result(error e) : is_ok(false), err(e) {}
        
        result(::zres r) : is_ok(r.is_ok) 
        {
            if (!is_ok) 
            {
                err = r.err;
            }
        }
        
        template <typename U>
        result(const result<U> &other) : is_ok(false) 
        {
            if (other.is_ok) 
            {
                ::zerr_panic("Attempted to convert success result<U> to error result<T>", __FILE__, __LINE__);
            }
            err = other.err;
        }
        
        result(const result &other) : is_ok(other.is_ok) 
        {
            if (is_ok) 
            {
                new (&val) T(other.val); 
            }
            else 
            {
                err = other.err;
            }
        }

        result(result &&other) noexcept : is_ok(other.is_ok) 
        {
            if (is_ok) 
            {
                new (&val) T(std::move(other.val)); 
            }
            else 
            {
                err = other.err;
            }
        }

        ~result() 
        { 
            if (is_ok) 
            {
                val.~T(); 
            }
        }

        bool ok() const 
        { 
            return is_ok; 
        }

        T &unwrap_val() 
        { 
            if (!is_ok) 
            { 
                ::zerr_print(err); 
                ::zerr_panic("unwrap()", __FILE__, __LINE__); 
            } 
            return val; 
        }

        operator ::zerr() const 
        { 
            return is_ok ? (::zerr){0, nullptr, nullptr, 0, nullptr, nullptr} : err; 
        }

        explicit operator bool() const 
        { 
            return is_ok; 
        }
    };

    template <>
    class result<void> 
    {
    public:
        bool is_ok;
        ::zerr err;
        
        result() : is_ok(true), err{0, nullptr, nullptr, 0, nullptr, nullptr} {}
        
        result(::zerr e) : is_ok(false), err(e) {}
        result(error e) : is_ok(false), err(e) {}
        static result<void> success() 
        { 
            return result(); 
        }
        
        bool ok() const 
        { 
            return is_ok; 
        }

        result(::zres r) : is_ok(r.is_ok) 
        {
            if (!is_ok) 
            {
                err = r.err;
            }
        }

        template <typename U>
        result(const result<U> &other) : is_ok(false) 
        {
            if (other.is_ok) 
            {
                ::zerr_panic("Attempted to convert success result<U> to error result<void>", __FILE__, __LINE__);
            }
            err = other.err;
        }

        operator ::zres() const 
        { 
            return { is_ok, err }; 
        }

        operator bool() const 
        { 
            return is_ok; 
        }
    };
}
#endif // __cplusplus

#endif // ZERROR_H

#ifdef ZERROR_IMPLEMENTATION

// Colors.
#if defined(ZERROR_NO_COLOR)
#   define ZERROR_COL_RED     ""
#   define ZERROR_COL_YEL     ""
#   define ZERROR_COL_GRY     ""
#   define ZERROR_COL_RST     ""
#   define ZERROR_COL_BG_RED  ""
#else
#   define ZERROR_COL_RED     "\033[1;31m"
#   define ZERROR_COL_YEL     "\033[0;33m"
#   define ZERROR_COL_GRY     "\033[0;90m"
#   define ZERROR_COL_RST     "\033[0m"
#   define ZERROR_COL_BG_RED  "\033[41;37m"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static ZERROR_THREAD_LOCAL char z_err_buffers[8][2048]; 
static ZERROR_THREAD_LOCAL int z_err_idx = 0;

static char* zerr_get_buf(void) 
{
    return z_err_buffers[z_err_idx++ & 7];
}

void zerr_print(zerr e) 
{
    fprintf(stderr, "\n%s[!] Error:%s %s\n", ZERROR_COL_RED, ZERROR_COL_RST, e.msg);
    fprintf(stderr, "    %sat%s %s (%s:%d) %s[Origin]%s\n", 
            ZERROR_COL_GRY, ZERROR_COL_RST,
            e.func ? e.func : "unknown", 
            e.file, 
            e.line,
            ZERROR_COL_YEL, ZERROR_COL_RST);
    if (e.source) 
    {
         fprintf(stderr, "    %s[Expr]%s %s\n", ZERROR_COL_GRY, ZERROR_COL_RST, e.source);
    }
    fprintf(stderr, "\n");
}

void zerr_panic(const char *msg, const char *file, int line) 
{
    fprintf(stderr, "\n%s[PANIC]%s %s\n", ZERROR_COL_BG_RED, ZERROR_COL_RST, msg);
    fprintf(stderr, "        at %s:%d\n\n", file, line);
    ZERROR_TRAP();
    ZERROR_PANIC_ACTION();
}

zerr zerr_create_impl(int code, const char *file, int line, const char *func, const char *fmt, ...)
{
    char *buf = zerr_get_buf();
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 2048, fmt, args);
    va_end(args);

    return (zerr){ .code = code, .msg = buf, .file = file, .line = line, .func = func, .source = NULL };
}

zerr zerr_errno_impl(int code, const char *file, int line, const char *func, const char *fmt, ...)
{
    char *buf = zerr_get_buf();
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, 2048, fmt, args);
    va_end(args);

    if (len < 2000) 
    {
        snprintf(buf + len, 2048 - len, ": %s", strerror(errno));
    }

    return (zerr){ .code = code, .msg = buf, .file = file, .line = line, .func = func, .source = NULL };
}

zerr zerr_add_trace(zerr e, const char *func, const char *file, int line) 
{
    char *buf = zerr_get_buf();
    
    int len = snprintf(buf, 2048, "%s", e.msg);
    if (len >= 2048) 
    {
        len = 2047;
    }

    if (len < 2000) 
    {
        snprintf(buf + len, 2048 - len, "\n    %sat%s %s (%s:%d)", ZERROR_COL_GRY, ZERROR_COL_RST, func, file, line);
    }
    
    return (zerr){ .code = e.code, .msg = buf, .file = e.file, .line = e.line, .func = e.func, .source = e.source };
}

zerr zerr_wrap(zerr e, const char *fmt, ...) 
{
    char *buf = zerr_get_buf();
    
    int len = snprintf(buf, 2048, "%s", e.msg);
    if (len >= 2048) 
    {
        len = 2047;
    }

    int remaining = 2048 - len - 1;
    if (remaining > 50) 
    { 
        strncat(buf, "\n  ", remaining);
        strncat(buf, ZERROR_COL_RED, remaining);
        strncat(buf, "|", remaining);
        strncat(buf, ZERROR_COL_RST, remaining);
        strncat(buf, " context: ", remaining);
        
        len = strlen(buf);
        remaining = 2048 - len - 1;

        if (remaining > 0) 
        {
            va_list args;
            va_start(args, fmt);
            vsnprintf(buf + len, remaining, fmt, args);
            va_end(args);
        }
    }
    return (zerr){ .code = e.code, .msg = buf, .file = e.file, .line = e.line, .func = e.func, .source = e.source };
}

int zerr_run(zres result) 
{
    if (!result.is_ok) 
    {
        zerr_print(result.err);
        return 1;
    }
    return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ZERROR_IMPLEMENTATION
