/*
 * Copyright 2026 davenonymous
 *
 * Jansson's private build configuration for the AMBuild build of SMJansson,
 * written by hand from jansson/cmake/jansson_private_config.h.cmake with the
 * values Jansson's CMake build detects on Linux with GCC or clang. The MSVC
 * branch follows the same template for Windows; it builds with Visual Studio
 * 2022 but has not been run on a server.
 * Jansson reads this file when HAVE_CONFIG_H is defined (see AMBuilder).
 */

#ifndef JANSSON_PRIVATE_CONFIG_H
#define JANSSON_PRIVATE_CONFIG_H

#ifdef _MSC_VER

#define HAVE_FCNTL_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_LOCALE_H 1
#define HAVE_SETLOCALE 1

#define HAVE_INT32_T 1
#define HAVE_UINT32_T 1
#define HAVE_UINT16_T 1
#define HAVE_UINT8_T 1

#include <stdint.h>
#define ssize_t intptr_t

#define USE_WINDOWS_CRYPTOAPI 1

#else

#define HAVE_ENDIAN_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SCHED_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_STDINT_H 1

#define HAVE_CLOSE 1
#define HAVE_GETPID 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_OPEN 1
#define HAVE_READ 1
#define HAVE_SCHED_YIELD 1

#define HAVE_SYNC_BUILTINS 1
#define HAVE_ATOMIC_BUILTINS 1

#define HAVE_LOCALE_H 1
#define HAVE_SETLOCALE 1

#define HAVE_INT32_T 1
#define HAVE_UINT32_T 1
#define HAVE_UINT16_T 1
#define HAVE_UINT8_T 1
#define HAVE_SSIZE_T 1

#define USE_URANDOM 1

#endif

/* Format reals with David M. Gay's dtoa (src/dtoa.c), Jansson's default. */
#define USE_DTOA 1
#define DTOA_ENABLED 1

/* New objects start with 2^3 hash buckets, Jansson's default. */
#define INITIAL_HASHTABLE_ORDER 3

#endif
