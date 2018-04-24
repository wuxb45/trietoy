// BEGIN HEAD (put this at the head of your code)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "trie.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

// global variables
static u64 __nr_samples = 0;
static u8 ** __samples = NULL;
static u32 * __sizes = NULL;

  static inline u64
xorshift(const u64 seed)
{
  u64 x = seed ? seed : 88172645463325252lu;
  x ^= x >> 12; // a
  x ^= x << 25; // b
  x ^= x >> 27; // c
  return x * UINT64_C(2685821657736338717);
}
static __thread u64 __random_seed_u64 = 0;

  static inline u64
random_u64(void)
{
  __random_seed_u64 = xorshift(__random_seed_u64);
  return __random_seed_u64;
}

  static u64
time_nsec(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return ts.tv_sec * UINT64_C(1000000000) + ts.tv_nsec;
}

  static double
time_sec(void)
{
  const u64 nsec = time_nsec();
  return ((double)nsec) / 1000000000.0;
}

  bool
load_trace(const char * const filename)
{
  char * buf = NULL;
  size_t size = 0;
  FILE * fin = fopen(filename, "r");
  if (fin == NULL) return false;
  u64 i = 0;
  // count for lines
  while (getline(&buf, &size, fin) >= 1) {
    i++;
  }
  rewind(fin);
  __samples = malloc(sizeof(char *) * i);
  __sizes = malloc(sizeof(u32) * i);
  __nr_samples = i;
  ssize_t len = 0;
  i = 0;
  while ((len = getline(&buf, &size, fin)) >= 1) {
    if (buf[len-1] == '\n') { // remove trailing '\n'
      len--;
      buf[len] = '\0';
    }
    __samples[i] = strdup(buf);
    __sizes[i] = len;
    i++;
  }
  free(buf);
  fclose(fin);
  return true;
}

  int
main(int argc, char ** argv)
{
  if (argc < 2) {
    printf("Usage: %s <samples>; # operations at stdin\n", argv[0]);
    return 0;
  }
  const bool r = load_trace(argv[1]);
  if (r == false) {
    printf("load_trace failed\n");
    return 0;
  }
  printf("number of keys: %lu\n", __nr_samples);
  struct trie * trie = trie_new();

  char * buf = NULL;
  size_t size = 0;
  while (getline(&buf, &size, stdin) >= 10) {
    u32 v1 = 0;
    u32 v2 = 0;
    sscanf(buf + 7, "%u%u", &v1, &v2);
    if (v1 >= __nr_samples) v1 = 0;
    if (v2 >= __nr_samples) v2 = 0;
    const u32 radius = v2 - v1 + 1;
    if (!radius) continue;
    u64 set_y = 0; // successful SET
    u64 set_n = 0; // failed SET (it may fail if there is no enough memory)
    u64 get_y = 0; // found
    u64 get_n = 0; // not found
    u64 del_y = 0; // deleted the key
    u64 del_n = 0; // nothing is deleted
    const double t0 = time_sec();
    if (memcmp(buf, "seqset", 6) == 0) {
      for (u32 i = v1; i <= v2; i++) {
        const bool rset = trie_set(trie, __sizes[i], __samples[i]);
        if (rset) { set_y++; } else { set_n++; }
      }

    } else if (memcmp(buf, "seqget", 6) == 0) {
      for (u32 i = v1; i <= v2; i++) {
        const bool rget = trie_get(trie, __sizes[i], __samples[i]);
        if (rget) { get_y++; } else { get_n++; }
      }

    } else if (memcmp(buf, "seqdel", 6) == 0) {
      for (u32 i = v1; i <= v2; i++) {
        const bool rdel = trie_del(trie, __sizes[i], __samples[i]);
        if (rdel) { del_y++; } else { del_n++; }
      }

    } else if (memcmp(buf, "rndset", 6) == 0) {
      for (u32 i = v1; i <= v2; i++) {
        const u32 r = v1 + (random_u64() % radius);
        const bool rset = trie_set(trie, __sizes[r], __samples[r]);
        if (rset) { set_y++; } else { set_n++; }
      }

    } else if (memcmp(buf, "rndget", 6) == 0) {
      for (u32 i = v1; i <= v2; i++) {
        const u32 r = v1 + (random_u64() % radius);
        const bool rget = trie_get(trie, __sizes[r], __samples[r]);
        if (rget) { get_y++; } else { get_n++; }
      }

    } else if (memcmp(buf, "rnddel", 6) == 0) {
      for (u32 i = v1; i <= v2; i++) {
        const u32 r = v1 + (random_u64() % radius);
        const bool rdel = trie_del(trie, __sizes[r], __samples[r]);
        if (rdel) { del_y++; } else { del_n++; }
      }

    }
    const double dt = time_sec() - t0;
    const double ops = ((double)(set_y + set_n + get_y + get_n + del_y + del_n)) / dt;
    printf("%s  set y %lu n %lu  get y %lu n %lu  del y %lu n %lu  req_per_sec %.3lf\n",
           buf, set_y, set_n, get_y, get_n, del_y, del_n, ops);
  }
  // clean up heap memory
  for (u64 i = 0; i < __nr_samples; i++) {
    free(__samples[i]);
  }
  free(__samples);
  free(__sizes);
  return 0;
}
