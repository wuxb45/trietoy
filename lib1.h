/*
 * Copyright (c) 2016-2018  Wu, Xingbo <wuxb45@gmail.com>
 *
 * All rights reserved. No warranty, explicit or implicit, provided.
 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// includes {{{
// C headers
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>

// POSIX headers
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

// Linux headers
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
// }}} includes

// types {{{
typedef int_least8_t            s8;
typedef int_least16_t           s16;
typedef int_least32_t           s32;
typedef int_least64_t           s64;

typedef uint_least8_t           u8;
typedef uint_least16_t          u16;
typedef uint_least32_t          u32;
typedef uint_least64_t          u64;
// }}} types

// timing {{{
  extern u64
rdtsc(void);

  extern u64
time_nsec(void);

  extern double
time_sec(void);

  extern u64
time_diff_nsec(const u64 last);

  extern double
time_diff_sec(const double last);

  extern u64
timespec_diff(const struct timespec t0, const struct timespec t1);
// }}} timing

// rgen {{{
  extern u64
xorshift(const u64 x);

  extern u64
random_u64(void);

  extern u64
srandom_u64(const u64 seed);

  extern double
random_double(void);
// }}} rgen

// kv {{{
struct kv {
  u32 klen;
  u32 vlen;
  u8 kv[];  // len(kv) == klen + vlen
};

  extern size_t
kv_size(const struct kv * const kv);

  extern size_t
kv_size_align(const struct kv * const kv, const u64 align);

  extern size_t
key_size(const struct kv * const key);

  extern size_t
key_size_align(const struct kv * const key, const u64 align);

  extern void
kv_refill(struct kv * const kv, const void * const key, const u32 klen, const void * const value, const u32 vlen);

  extern void
kv_refill_str(struct kv * const kv, const char * const key, const char * const value);

  extern struct kv *
kv_create(const void * const key, const u32 klen, const void * const value, const u32 vlen);

  extern struct kv *
kv_create_str(const char * const key, const char * const value);

  extern struct kv *
kv_dup(const struct kv * const kv);

  extern struct kv *
kv_dup_key(const struct kv * const kv);

  extern struct kv *
kv_dup2(const struct kv * const from, struct kv * const to);

  extern struct kv *
kv_dup2_key(const struct kv * const from, struct kv * const to);

  extern struct kv *
kv_dup2_key_prefix(const struct kv * const from, struct kv * const to, const u64 plen);

  extern bool
kv_keymatch(const struct kv * const key1, const struct kv * const key2);

  extern bool
kv_keymatch_r(const struct kv * const key1, const struct kv * const key2);

  extern bool
kv_fullmatch(const struct kv * const kv1, const struct kv * const kv2);

typedef int  (*kv_compare_func)(const struct kv * const kv1, const struct kv * const kv2);

  extern int
kv_keycompare(const struct kv * const kv1, const struct kv * const kv2);

  extern void
kv_qsort(const struct kv ** const kvs, const size_t nr);

  extern void *
kv_value_ptr(struct kv * const kv);

  extern void *
kv_key_ptr(struct kv * const kv);

  extern const void *
kv_value_ptr_const(const struct kv * const kv);

  extern const void *
kv_key_ptr_const(const struct kv * const kv);

  extern u32
kv_key_lcp(const struct kv * const key1, const struct kv * const key2);

  extern bool
kv_key_is_prefix(const struct kv * const p, const struct kv * const key);

  extern void
kv_print(const struct kv * const kv, const u32 nv, FILE * const out);

  extern void
kv_print_key(const struct kv * const kv, FILE * const out);
// }}} kv

#ifdef __cplusplus
}
#endif
// vim:fdm=marker
