#ifndef MACROS_H
#define MACROS_H

#ifdef _STDIO_H_
#error must include macros.h before stdio.h
#endif

#define _GNU_SOURCE
#define _ISOC11_SOURCE

#include "pretty_ansi.h"

#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#define aligned_alloc_shim(alignment, size) ({ void * _tmp = NULL; int _tmp2 __attribute((unused)) = posix_memalign(&_tmp, alignment, size); _tmp; })

#define SWAP(x, y) do { __auto_type _tmp = x; x = y; y = _tmp; } while (0)

#define SHIT(...) do { fprintf(stderr, ERROR_ANSI " " __VA_ARGS__); exit(EXIT_FAILURE); } while(0)
#define NOPE SHIT

#define alloc_sprintf(...) ({ char * _tmp; if (asprintf(&_tmp, __VA_ARGS__) <= 0) abort(); _tmp ; })
#define sprintf_and_system(...) ({ char * _c = alloc_sprintf(__VA_ARGS__); int _ret = system(_c); free(_c); _ret; })

#define fopen_checked(path, mode) ({ \
const char * _path = path, * _mode = mode;\
FILE * _tmp = fopen(_path, _mode);\
if (!_tmp) { \
char _in_main = !!strstr(__func__, "main");\
char * _cmd = getenv("_");\
fprintf(stderr, ERROR_ANSI " %s%s%s: cannot %s %s: %s\n", (_in_main && _cmd) ? basename(_cmd) : "", _cmd && !_in_main ? ": " : "", _in_main ? "" : __func__,\
strchr(_mode, 'r') ? "read from" : "write to", _path, strerror(errno));\
exit(EXIT_FAILURE);\
}\
_tmp;\
})

#define sprintf_and_fopen_for_writing(...) ({ \
char * _tmppath; \
if (asprintf(&_tmppath, __VA_ARGS__) <= 0) abort(); \
FILE * _fh = fopen_checked(_tmppath, "w");\
free (_tmppath); \
_fh; \
})

#endif

#define CLAMP(ix, X) ({ const ssize_t _ix = ix; const size_t _X = X; _ix < 0 ? 0 : (size_t)_ix > _X ? _X : (size_t)_ix; })

#define ERROR_CALL(x) do { fprintf(stderr, ERROR_ANSI " %s: %s failed: %s\n", __func__, x, strerror(errno)); exit(EXIT_FAILURE); } while (0)
#define ERROR_CALL_PATH(x, y) do { fprintf(stderr, ERROR_ANSI " %s: could not %s %s: %s\n", __func__, x, y, strerror(errno)); exit(EXIT_FAILURE); } while (0)
