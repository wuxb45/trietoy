#include "lib1.h"
#include "trie.h"

// assume _out_ is large enough!
  static void
magic_fill_kv(const u64 r, struct kv * const out)
{
  const u32 klen = ((r & 0x3lu) << 3) + 8;        // klen can be 8, 16, 24, 32
  const u32 vlen = (((r >> 2) & 0x3lu) << 3) + 8; // vlen can be 8, 16, 24, 32
  out->klen = klen;
  out->vlen = vlen;
  u64 * iter = (u64 *)out->kv;
  for (u64 i = 0; i < (klen + vlen); i += 8) {
    *iter = (r ^ i);
    iter++;
  }
}

  int
main(int argc, char ** argv)
{
  if (argc < 2) {
    printf("Usage: %s <nr-keys>\n", argv[0]);
    exit(0);
  }
  const u64 NR = (u64)strtoull(argv[1], NULL, 10);
  double t0;
  // (1)
  struct trie * const t1 = trie_new();
  if (t1 == NULL) {
    printf("trie_new() failed\n");
    exit(0);
  }

  // prepare a set of random numbers
  u64 * const m = malloc(sizeof(*m) * NR);
  for (u64 i = 0; i < NR; i++) {
    m[i] = random_u64();
  }
  const u64 * const magicnums = m; // "freeze" it
  // fill a lot of KVs
  struct kv * const tmp = malloc(1000);
  t0 = time_sec();
  for (u64 i = 0; i < NR; i++) {
    const u64 r = magicnums[i];
    magic_fill_kv(r, tmp);
    if (trie_set(t1, tmp) == false) {
      printf("ERROR with trie_set(), r = %lu\n", r);
      exit(0);
    }
  }
  const double dt_set = time_diff_sec(t0);

  struct kv * const out = malloc(1000);
  // read them out to verify
  for (u64 i = 0; i < NR; i++) {
    const u64 r = magicnums[i];
    magic_fill_kv(r, tmp);
    if (NULL == trie_get(t1, tmp, out)) {
      printf("ERROR with trie_get(), NULL returned\n");
      exit(0);
    }
    magic_fill_kv(r, tmp); // Bonus point: why this fill?
    if (kv_fullmatch(tmp, out) == false) {
      printf("ERROR with trie_get(), tmp != out\n");
      exit(0);
    }
  }

  // perf get
  t0 = time_sec();
  for (u64 i = 0; i < NR; i++) {
    const u64 r = magicnums[i];
    magic_fill_kv(r, tmp);
    (void)trie_get(t1, tmp, out);
  }
  const double dt_get = time_diff_sec(t0);

  // delete all
  u64 ndel = 0;
  t0 = time_sec();
  for (u64 i = 0; i < NR; i++) {
    const u64 r = magicnums[i];
    magic_fill_kv(r, tmp);
    if (trie_del(t1, tmp)) {
      ndel++;
    }
  }
  const double dt_del = time_diff_sec(t0);
  printf("%lu deleted\n", ndel);
  printf("dt_set %.3lfs; dt_get %.3lfs; dt_del %.3lfs\n", dt_set, dt_get, dt_del);
  trie_destroy(t1);
  return 0;
}
