
#ifndef ZERROR_H
#define ZERROR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -> Configuration. */

/* Feature detection. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define Z_TYPEOF(x) typeof(x)
    #define Z_HAS_MODERN_C 1
#elif defined(__GNUC__) || defined(__clang__)
    #define Z_TYPEOF(x) __typeof__(x)
    #define Z_HAS_MODERN_C 1
#else
    #define Z_HAS_MODERN_C 0
#endif

/* Thread local storage. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
    #define Z_THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
    #define Z_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__)
    #define Z_THREAD_LOCAL __thread
#else
    #define Z_THREAD_LOCAL
#endif

#define Z_CONCAT_(a, b) a ## b
#define Z_CONCAT(a, b) Z_CONCAT_(a, b)
#define Z_UID(prefix) Z_CONCAT(prefix, __LINE__)

/* Debug trap. */
#if defined(Z_DEBUG)
    #if defined(_MSC_VER)
        #define Z_TRAP() __debugbreak()
    #elif defined(__GNUC__) || defined(__clang__)
        #define Z_TRAP() __builtin_trap()
    #else
        #define Z_TRAP() ((void)0)
    #endif
#else
    #define Z_TRAP() ((void)0)
#endif

/* Core error type. */
typedef struct 
{
    int code;
    const char *msg;
    const char *file;
    int line;
    const char *func;
} zerr;

zerr zerr_create_impl(int code, const char *file, int line, const char *func, const char *fmt, ...);

/* * Create Error: Captures location, traps debugger (if Z_DEBUG), and formats message.
 * Usage: zerr_create(404, "User %d not found", uid); 
 */
#define zerr_create(code, ...) \
    (Z_TRAP(), zerr_create_impl((code), __FILE__, __LINE__, __func__, __VA_ARGS__))

/* Helper to attach trace info if enabled. */
zerr zerr_add_trace(zerr e, const char *func, const char *file, int line);

void zerr_print(zerr e);
void zerr_panic(const char *msg, const char *file, int line);
zerr zerr_wrap(zerr e, const char *fmt, ...);

static inline zerr zerr_with_src(zerr e, const char *src) { return e; } // Legacy compat

/* Result type generators. */
#define DEFINE_RESULT(T, Name)                                  \
    typedef struct {                                            \
        bool is_ok;                                             \
        union { T val; zerr err; };                             \
    } Name;                                                     \
    static inline Name Name##_ok(T v) {                         \
        Name r; r.is_ok = true; r.val = v; return r;            \
    }                                                           \
    static inline Name Name##_err(zerr e) {                     \
        Name r; r.is_ok = false; r.err = e; return r;           \
    }

typedef struct { bool is_ok; zerr err; } zres;
static inline zres zres_ok(void)    { return (zres){ .is_ok = true }; }
static inline zres zres_err(zerr e) { return (zres){ .is_ok = false, .err = e }; }

// Some predefined common results.
DEFINE_RESULT(int,      ResInt)
DEFINE_RESULT(float,    ResFloat)
DEFINE_RESULT(double,   ResDouble)
DEFINE_RESULT(bool,     ResBool)
DEFINE_RESULT(size_t,   ResSize)
DEFINE_RESULT(void*,    ResPtr)
DEFINE_RESULT(char*,    ResStr)

/* --- Trace Logic --- */
#ifdef Z_ENABLE_TRACE
    #define Z_TRACE_OP(e) zerr_add_trace(e, __func__, __FILE__, __LINE__)
#else
    #define Z_TRACE_OP(e) (e)
#endif

/* --- Macros & Flow Control --- */

#define Z_CHECK(expr, src)                                      \
    do {                                                        \
        Z_TYPEOF(expr) Z_UID(_r) = (expr);                      \
        if (!Z_UID(_r).is_ok) {                                 \
            Z_UID(_r).err = Z_TRACE_OP(Z_UID(_r).err);          \
            return zres_err(Z_UID(_r).err);                     \
        }                                                       \
    } while(0)

#define Z_CHECK_INTO(RetType, expr, src)                        \
    do {                                                        \
        Z_TYPEOF(expr) Z_UID(_r) = (expr);                      \
        if (!Z_UID(_r).is_ok) {                                 \
            Z_UID(_r).err = Z_TRACE_OP(Z_UID(_r).err);          \
            return RetType##_err(Z_UID(_r).err);                \
        }                                                       \
    } while(0)

#define Z_CHECK_WRAP(expr, fmt, ...)                                        \
    do {                                                                    \
        Z_TYPEOF(expr) Z_UID(_r) = (expr);                                  \
        if (!Z_UID(_r).is_ok) {                                             \
            Z_UID(_r).err = Z_TRACE_OP(Z_UID(_r).err);                      \
            return zres_err(zerr_wrap(Z_UID(_r).err, fmt, ##__VA_ARGS__));  \
        }                                                                   \
    } while(0)

#define Z_CHECK_CTX(expr, fmt, ...)                                         \
    do {                                                                    \
        Z_TYPEOF(expr) Z_UID(_r) = (expr);                                  \
        if (!Z_UID(_r).is_ok) {                                             \
            Z_UID(_r).err = Z_TRACE_OP(Z_UID(_r).err);                      \
            Z_UID(_r).err = zerr_wrap(Z_UID(_r).err, fmt, ##__VA_ARGS__);   \
            return zres_err(Z_UID(_r).err);                                 \
        }                                                                   \
    } while(0)

#define Z_ENSURE(cond, code, msg)                               \
    do {                                                        \
        if (!(cond)) {                                          \
            return zres_err(zerr_create((code), (msg)));        \
        }                                                       \
    } while(0)

#define Z_ENSURE_INTO(RetType, cond, code, msg)                 \
    do {                                                        \
        if (!(cond)) {                                          \
            return RetType##_err(zerr_create((code), (msg)));   \
        }                                                       \
    } while(0)

#if Z_HAS_MODERN_C
    
    #define Z_TRY(expr, src)                                            \
        ({  Z_TYPEOF(expr) Z_UID(_res) = (expr);                        \
            if (!Z_UID(_res).is_ok) {                                   \
                Z_UID(_res).err = Z_TRACE_OP(Z_UID(_res).err);          \
                return Z_UID(_res);                                     \
            }                                                           \
            Z_UID(_res).val;                                            \
        })

    #define Z_TRY_INTO(RetType, expr, src)                              \
        ({  Z_TYPEOF(expr) Z_UID(_res) = (expr);                        \
            if (!Z_UID(_res).is_ok) {                                   \
                Z_UID(_res).err = Z_TRACE_OP(Z_UID(_res).err);          \
                return RetType##_err(Z_UID(_res).err);                  \
            }                                                           \
            Z_UID(_res).val;                                            \
        })

    #define Z_TRY_PTR(RetType, expr, code, msg)                     \
        ({  Z_TYPEOF(expr) Z_UID(_p) = (expr);                      \
            if (Z_UID(_p) == NULL) {                                \
                return RetType##_err(zerr_create((code), (msg)));   \
            }                                                       \
            Z_UID(_p);                                              \
        })

    #define Z_TRY_OR(expr, default_val)                             \
        ({                                                          \
            Z_TYPEOF(expr) Z_UID(_res) = (expr);                    \
            Z_UID(_res).is_ok ? Z_UID(_res).val : (default_val);    \
        })

    #define Z_EXPECT(expr, msg)                         \
        ({  Z_TYPEOF(expr) Z_UID(_res) = (expr);        \
            if (!Z_UID(_res).is_ok) {                   \
                zerr_print(Z_UID(_res).err);            \
                zerr_panic(msg, __FILE__, __LINE__);    \
            }                                           \
            Z_UID(_res).val;                            \
        })

    #define Z_DEFER_HK(l, c)                                \
        void Z_CONCAT(z_defer_fn_, l)(void *_) { c; }       \
        __attribute__((cleanup(Z_CONCAT(z_defer_fn_, l))))  \
        char Z_CONCAT(z_defer_var_, l)
    
    #define z_defer(code) Z_DEFER_HK(__LINE__, code)

#endif

/* Short macros. */
#ifdef Z_SHORT_ERR
    #define check(expr)             Z_CHECK(expr, #expr)
    #define check_into(T, expr)     Z_CHECK_INTO(T, expr, #expr)
    #define check_wrap(expr, ...)   Z_CHECK_WRAP(expr, __VA_ARGS__)
    #define check_ctx(expr, ...)    Z_CHECK_CTX(expr, __VA_ARGS__)
    
    #define ensure(c, code, m)          Z_ENSURE(c, code, m)
    #define ensure_into(T, c, code, m)  Z_ENSURE_INTO(T, c, code, m)

    #if Z_HAS_MODERN_C
        #define try(expr)               Z_TRY(expr, #expr)
        #define try_into(T, expr)       Z_TRY_INTO(T, expr, #expr)
        #define try_ptr(T, p, c, m)     Z_TRY_PTR(T, p, c, m)
        #define try_or(e, d)            Z_TRY_OR(e, d)

        #define unwrap(e)               Z_EXPECT(e, "unwrap() failed")
        #define expect(e, m)            Z_EXPECT(e, m)

        #define defer(code)             z_defer(code)
    #endif

    #define run(expr)  ({ zres _r = (expr); if (!_r.is_ok) zerr_print(_r.err); _r.is_ok ? 0 : 1; })
#endif

#ifdef ZERROR_IMPLEMENTATION

static Z_THREAD_LOCAL char z_err_buffers[8][2048]; 
static Z_THREAD_LOCAL int z_err_idx = 0;

static char* zerr_get_buf(void) {
    return z_err_buffers[z_err_idx++ & 7];
}

void zerr_print(zerr e) 
{
    fprintf(stderr, "\n\033[1;31m[!] Error:\033[0m %s\n", e.msg);
    fprintf(stderr, "    \033[0;90mat\033[0m %s (%s:%d) \033[0;33m[Origin]\033[0m\n", 
            e.func ? e.func : "unknown", 
            e.file, 
            e.line);
    fprintf(stderr, "\n");
}

void zerr_panic(const char *msg, const char *file, int line) 
{
    fprintf(stderr, "\n\033[41;37m[PANIC]\033[0m %s\n", msg);
    fprintf(stderr, "        at %s:%d\n\n", file, line);
    abort();
}

zerr zerr_create_impl(int code, const char *file, int line, const char *func, const char *fmt, ...)
{
    char *buf = zerr_get_buf();
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 2048, fmt, args);
    va_end(args);

    return (zerr){ .code = code, .msg = buf, .file = file, .line = line, .func = func };
}

zerr zerr_add_trace(zerr e, const char *func, const char *file, int line) 
{
    char *buf = zerr_get_buf();
    
    int len = snprintf(buf, 2048, "%s", e.msg);
    if (len >= 2048) len = 2047;

    if (len < 2000) {
        snprintf(buf + len, 2048 - len, "\n    \033[0;90mat\033[0m %s (%s:%d)", func, file, line);
    }
    
    return (zerr){ .code = e.code, .msg = buf, .file = e.file, .line = e.line, .func = e.func };
}

zerr zerr_wrap(zerr e, const char *fmt, ...) 
{
    char *buf = zerr_get_buf();
    
    int len = snprintf(buf, 2048, "%s", e.msg);
    if (len >= 2048) len = 2047;

    int remaining = 2048 - len - 1;
    if (remaining > 50) 
    { 
        strncat(buf, "\n  \033[1;31m|\033[0m context: ", remaining);
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
    return (zerr){ .code = e.code, .msg = buf, .file = e.file, .line = e.line, .func = e.func };
}
#endif // ZERROR_IMPLEMENTATION 
#ifdef __cplusplus
}
#endif // ZERROR_H
#endif
