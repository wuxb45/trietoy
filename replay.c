// BEGIN HEAD (put this at the head of your code)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "trie.h"

#define SET ((1))
#define GET ((2))
#define DEL ((3))
#define PRT ((4))  // print results

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

struct access {
  u32 op; // SET/GET/DEL
  u32 len; // length of the key in bytes
  u8 * key; // the key;
};

char __op_map[5][4] = {"???", "SET", "GET", "DEL", "PRT"};
// END HEAD

// global variables
static u64 __nr_access = 0;
static struct access * __workload = NULL;

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
  while (getline(&buf, &size, fin) >= 4) {
    i++;
  }
  rewind(fin);
  __workload = malloc(sizeof(struct access) * i);
  __nr_access = i;
  i = 0;
  while (getline(&buf, &size, fin) >= 3) {
    __workload[i].len = 0;
    __workload[i].key = NULL;
    if (strncmp(buf, "SET", 3) == 0) {
      __workload[i].op = SET;
    } else if (strncmp(buf, "GET", 3) == 0) {
      __workload[i].op = GET;
    } else if (strncmp(buf, "DEL", 3) == 0) {
      __workload[i].op = DEL;
    } else if (strncmp(buf, "PRT", 3) == 0) {
      __workload[i].op = PRT;
      i++;
      continue; // no key for print
    } else {
      __workload[i].op = 0; // invalid op
      // ignore this line
      continue;
    }
    if (strlen(buf) < 4) continue;
    char * key = buf + 4;
    u32 len = strlen(key);
    if (key[len-1] == '\n') {
      key[len-1] = '\0'; // remove \newline
      len--;
    }
    __workload[i].len = len;
    __workload[i].key = (u8 *)strdup(key);
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
    printf("Usage: %s <filename>\n", argv[0]);
    return 0;
  }
  const bool r = load_trace(argv[1]);
  if (r == false) {
    printf("load_trace failed\n");
    return 0;
  }
  struct trie * trie = trie_new();
  u64 set_y = 0; // successful SET
  u64 set_n = 0; // failed SET (it may fail if there is no enough memory)
  u64 get_y = 0; // found
  u64 get_n = 0; // not found
  u64 del_y = 0; // deleted the key
  u64 del_n = 0; // nothing is deleted
  double t0 = time_sec();

  for (u64 i = 0; i < __nr_access; i++) {
    struct access * acc = &__workload[i];
    // TODO: the printf here can be removed once you read it
    // the safe use of %s: %.*s takes two arguments: [1] string length, [2] the string
    if (acc->op != PRT) {
      printf("[%s] [%.*s] head %c tail %c\n",
          __op_map[acc->op], acc->len, acc->key, acc->key[0], acc->key[acc->len - 1]);
    }
    switch (acc->op) {
      case SET:
        {
          const bool rset = trie_set(trie, acc->len, acc->key);
          if (rset) {
            set_y++;
          } else {
            set_n++;
          }
        }
        break;
      case GET:
        {
          const bool rget = trie_get(trie, acc->len, acc->key);
          if (rget) {
            get_y++;
          } else {
            get_n++;
          }
        }
        break;
      case DEL:
        {
          const bool rdel = trie_del(trie, acc->len, acc->key);
          if (rdel) {
            del_y++;
          } else {
            del_n++;
          }
        }
        break;
      case PRT:
        {
          const double dt = time_sec() - t0;
          const double ops = ((double)(set_y + set_n + get_y + get_n + del_y + del_n)) / dt;
          printf("set y %lu n %lu  get y %lu n %lu  del y %lu n %lu  req_per_sec %.3lf\n",
              set_y, set_n, get_y, get_n, del_y, del_n, ops);
          set_y = 0; set_n = 0;
          get_y = 0; get_n = 0;
          del_y = 0; del_n = 0;
          t0 = time_sec();
        }
      default: break;
    }
  }
  // clean up heap memory
  for (u64 i = 0; i < __nr_access; i++) {
    free(__workload[i].key);
  }
  free(__workload);
  return 0;
}
