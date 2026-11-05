/*
 * Copyright 2026 davenonymous
 *
 * Jansson's public configuration header for the AMBuild build of SMJansson,
 * written by hand from jansson/src/jansson_config.h.in in place of the one
 * Jansson's own configure or CMake would generate. Kept outside jansson/ so the
 * vendored release stays identical to upstream.
 */

#ifndef JANSSON_CONFIG_H
#define JANSSON_CONFIG_H

#ifdef __cplusplus
#define JSON_INLINE inline
#elif defined(_MSC_VER)
#define JSON_INLINE __inline
#else
#define JSON_INLINE inline
#endif

/* long long is available with every supported compiler. */
#define JSON_INTEGER_IS_LONG_LONG 1

/* Atomic reference counting where the compiler offers it. */
#if defined(__GNUC__) || defined(__clang__)
#define JSON_HAVE_ATOMIC_BUILTINS 1
#define JSON_HAVE_SYNC_BUILTINS 1
#else
#define JSON_HAVE_ATOMIC_BUILTINS 0
#define JSON_HAVE_SYNC_BUILTINS 0
#endif

/* Maximum recursion depth for parsing JSON input. */
#define JSON_PARSER_MAX_DEPTH 2048

#endif
