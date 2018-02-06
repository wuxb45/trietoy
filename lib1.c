/*
 * Copyright (c) 2016-2018  Wu, Xingbo <wuxb45@gmail.com>
 *
 * All rights reserved. No warranty, explicit or implicit, provided.
 */
#define _GNU_SOURCE

// headers {{{
#include "lib1.h"
#include <assert.h>
#include <math.h>
#include <execinfo.h>
#include <signal.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <netdb.h>
#include <alloca.h>
#include <nmmintrin.h>
#include <immintrin.h>
// }}} headers

// timing {{{
  inline u64
rdtsc(void)
{
  u32 hi, lo;
  asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
  return (((u64)hi) << 32) | (u64)lo;
}

  inline u64
time_nsec(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return ts.tv_sec * UINT64_C(1000000000) + ts.tv_nsec;
}

  inline double
time_sec(void)
{
  const u64 nsec = time_nsec();
  return ((double)nsec) / 1000000000.0;
}

  inline u64
time_diff_nsec(const u64 last)
{
  return time_nsec() - last;
}

  inline double
time_diff_sec(const double last)
{
  return time_sec() - last;
}

  inline u64
timespec_diff(const struct timespec t0, const struct timespec t1)
{
  return UINT64_C(1000000000) * (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec);
}
// }}} timing

// rgen {{{
  inline u64
xorshift(const u64 seed)
{
  u64 x = seed ? seed : 88172645463325252lu;
  x ^= x >> 12; // a
  x ^= x << 25; // b
  x ^= x >> 27; // c
  return x * UINT64_C(2685821657736338717);
}

static __thread u64 __random_seed_u64 = 0;

  inline u64
random_u64(void)
{
  __random_seed_u64 = xorshift(__random_seed_u64);
  return __random_seed_u64;
}

  inline u64
srandom_u64(const u64 seed)
{
  __random_seed_u64 = xorshift(seed);
  return __random_seed_u64;
}

#define RAND64_MAX   ((UINT64_C(0xffffffffffffffff)))
#define RAND64_MAX_D ((double)(RAND64_MAX))

  inline double
random_double(void)
{
  // random between 0.0 - 1.0
  const u64 r = random_u64();
  return ((double)r) / RAND64_MAX_D;
}
// }}} rgen

// kv {{{
  inline size_t
kv_size(const struct kv * const kv)
{
  return sizeof(*kv) + kv->klen + kv->vlen;
}

  inline size_t
kv_size_align(const struct kv * const kv, const u64 align)
{
  return (sizeof(*kv) + kv->klen + kv->vlen + (align - 1)) & (~(align - 1));
}

  inline size_t
key_size(const struct kv *const key)
{
  return sizeof(*key) + key->klen;
}

  inline size_t
key_size_align(const struct kv *const key, const u64 align)
{
  return (sizeof(*key) + key->klen + (align - 1)) & (~(align - 1));
}

// unsafe!
  inline void
kv_refill(struct kv * const kv, const void * const key, const u32 klen,
    const void * const value, const u32 vlen)
{
  if (kv) {
    kv->klen = klen;
    kv->vlen = vlen;
    memcpy(&(kv->kv[0]), key, klen);
    memcpy(&(kv->kv[klen]), value, vlen);
  }
}

// unsafe!
  inline void
kv_refill_str(struct kv * const kv, const char * const key, const char * const value)
{
  kv_refill(kv, key, (u32)strlen(key), value, (u32)strlen(value));
}

  inline struct kv *
kv_create(const void * const key, const u32 klen, const void * const value, const u32 vlen)
{
  struct kv * const kv = malloc(sizeof(*kv) + klen + vlen);
  kv_refill(kv, key, klen, value, vlen);
  return kv;
}

  inline struct kv *
kv_create_str(const char * const key, const char * const value)
{
  return kv_create(key, (u32)strlen(key), value, (u32)strlen(value));
}

  inline struct kv *
kv_dup(const struct kv * const kv)
{
  if (kv == NULL) return NULL;

  const size_t sz = kv_size(kv);
  struct kv * const new = (typeof(new))malloc(sz);
  if (new) {
    memcpy(new, kv, sz);
  }
  return new;
}

  inline struct kv *
kv_dup_key(const struct kv * const kv)
{
  if (kv == NULL) return NULL;

  const size_t sz = key_size(kv);
  struct kv * const new = (typeof(new))malloc(sz);
  if (new) {
    memcpy(new, kv, sz);
  }
  return new;
}

  inline struct kv *
kv_dup2(const struct kv * const from, struct kv * const to)
{
  if (from == NULL) return NULL;
  const size_t sz = kv_size(from);
  struct kv * const new = to ? to : (typeof(new))malloc(sz);
  memcpy(new, from, sz);
  return new;
}

  inline struct kv *
kv_dup2_key(const struct kv * const from, struct kv * const to)
{
  if (from == NULL) return NULL;
  const size_t sz = key_size(from);
  struct kv * const new = to ? to : (typeof(new))malloc(sz);
  memcpy(new, from, sz);
  new->vlen = 0;
  return new;
}

  inline struct kv *
kv_dup2_key_prefix(const struct kv * const from, struct kv * const to, const u64 plen)
{
  if (from == NULL) return NULL;
  const size_t sz = key_size(from) - from->klen + plen;
  struct kv * const new = to ? to : (typeof(new))malloc(sz);
  if (new) {
    new->klen = plen;
    memcpy(new->kv, from->kv, plen);
    new->vlen = 0;
  }
  return new;
}

// internal use for kv_keymatch
  static bool
kv_keymatch_key(const struct kv * const key1, const struct kv * const key2)
{
  u32 klen = key1->klen;
  const u8 * p1 = key1->kv;
  const u8 * p2 = key2->kv;
  while (klen >= sizeof(u64)) {
    if (*((const u64 *)p1) != *((const u64 *)p2)) return false;
    p1 += sizeof(u64);
    p2 += sizeof(u64);
    klen -= sizeof(u64);
  }
  if (klen & sizeof(u32)) {
    if (*((const u32 *)p1) != *((const u32 *)p2)) return false;
    p1 += sizeof(u32);
    p2 += sizeof(u32);
  }
  if (klen & sizeof(u16)) {
    if (*((const u16 *)p1) != *((const u16 *)p2)) return false;
    p1 += sizeof(u16);
    p2 += sizeof(u16);
  }
  if (klen & sizeof(u8)) {
    if ((*p1) != (*p2)) return false;
  }
  return true;
}

// key1 and key2 must be valid ptr
  inline bool
kv_keymatch(const struct kv * const key1, const struct kv * const key2)
{
  if ((key1->klen == key2->klen)) {
    return kv_keymatch_key(key1, key2);
  }
  return false;
}

  inline bool
kv_fullmatch(const struct kv * const kv1, const struct kv * const kv2)
{
  if ((!kv1) || (!kv2) || (kv1->klen != kv2->klen) || (kv1->vlen != kv2->vlen)) {
    return false;
  }
  const int cmp = memcmp(kv1->kv, kv2->kv, (size_t)(kv1->klen + kv1->vlen));
  return (cmp == 0) ? true : false;
}

  int
kv_keycompare(const struct kv * const kv1, const struct kv * const kv2)
{
  if (kv1 == NULL) {
    return (kv2 == NULL) ? 0 : -1;
  } else if (kv2 == NULL) {
    return 1;
  }
  const u32 len = kv1->klen < kv2->klen ? kv1->klen : kv2->klen;
  const int cmp = memcmp(kv1->kv, kv2->kv, (size_t)len);
  if (cmp == 0) {
    if (kv1->klen < kv2->klen) {
      return -1;
    } else if (kv1->klen > kv2->klen) {
      return 1;
    } else {
      return 0;
    }
  } else {
    return cmp;
  }
}

  static int
__kv_compare_pp(const void * const p1, const void * const p2)
{
  const struct kv ** const pp1 = (typeof(pp1))p1;
  const struct kv ** const pp2 = (typeof(pp2))p2;
  return kv_keycompare(*pp1, *pp2);
}

  inline void
kv_qsort(const struct kv ** const kvs, const size_t nr)
{
  qsort(kvs, nr, sizeof(kvs[0]), __kv_compare_pp);
}

  inline void *
kv_value_ptr(struct kv * const kv)
{
  return (void *)(&(kv->kv[kv->klen]));
}

  inline void *
kv_key_ptr(struct kv * const kv)
{
  return (void *)(&(kv->kv[0]));
}

  inline const void *
kv_value_ptr_const(const struct kv * const kv)
{
  return (const void *)(&(kv->kv[kv->klen]));
}

  inline const void *
kv_key_ptr_const(const struct kv * const kv)
{
  return (const void *)(&(kv->kv[0]));
}

// return the length of longest common prefix of the two keys
  inline u32
kv_key_lcp(const struct kv * const key1, const struct kv * const key2)
{
  const u32 max = (key1->klen < key2->klen) ? key1->klen : key2->klen;
  const u32 max64 = max & (~0x7u);
  u32 clen = 0;
  const u8 * p1 = key1->kv;
  const u8 * p2 = key2->kv;
  while ((clen < max64) && (*(const u64 *)p1 == *(const u64 *)p2)) {
    clen += sizeof(u64);
    p1 += sizeof(u64);
    p2 += sizeof(u64);
  }
  while ((clen < max) && (*p1 == *p2)) {
    clen++;
    p1++;
    p2++;
  }
  return clen;
}

// return true if p is a prefix of key
  inline bool
kv_key_is_prefix(const struct kv * const p, const struct kv * const key)
{
  return ((p->klen <= key->klen) && (kv_key_lcp(p, key) == p->klen)) ? true : false;
}

  void
kv_print(const struct kv * const kv, const u32 nv, FILE * const out)
{
  const u32 klen = kv->klen;
  fprintf(out, "kv k[%2u]", klen);
  for (u32 i = 0; i < klen; i++) {
    fprintf(out, " %02hhx", kv->kv[i]);
  }
  fprintf(out, "  v[%u]", kv->vlen);
  for (u32 i = 0; i < nv; i++) {
    fprintf(out, " %02hhx", kv->kv[klen + i]);
  }
  if (nv < kv->vlen) {
    fprintf(out, " ...");
  }
}

  void
kv_print_key(const struct kv * const kv, FILE * const out)
{
  const u32 klen = kv->klen;
  fprintf(out, "kv k[%2u]", klen);
  //// for binary keys (default)
  for (u32 i = 0; i < klen; i++) {
    fprintf(out, " %02hhx", kv->kv[i]);
  }
  //// for string keys
  //fprintf(out, "[%.*s]", (int)kv->klen, kv->kv);
}
// }}} kv

// fdm: marker
