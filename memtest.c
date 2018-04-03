// BEGIN HEAD (put this at the head of your code)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/resource.h>
#include "trie.h"

  int
main(int argc, char ** argv)
{
  if (argc < 2) {
    printf("Usage: %s <nr-operations>\n", argv[0]);
    return 0;
  }
  int nr = atoi(argv[1]);

  struct trie * const trie = trie_new();
  u8 key[1024];

  // for get memory usage
  struct rusage rs;
  getrusage(RUSAGE_SELF, &rs);
  const u64 kb0 = rs.ru_maxrss;

  for (int i = 0; i < nr; i++) {
    const u32 klen = ((u32)random()) % 1024;
    for (u32 k = 0; k < klen; k++) {
      key[k] = (u8)random();
    }
    trie_set(trie, klen, key);
    trie_del(trie, klen, key);
  }

  getrusage(RUSAGE_SELF, &rs);
  const u64 kb1 = rs.ru_maxrss;
  // get the delta
  printf("increased memory usage +%lu kB\n", kb1 - kb0);
  return 0;
}
